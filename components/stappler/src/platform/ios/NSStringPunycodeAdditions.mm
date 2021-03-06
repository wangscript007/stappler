//
//  NSStringPunycodeAdditions.m
//  Punycode
//
//  Created by Wevah on 2005.11.02.
//  Copyright 2005-2012 Derailer. All rights reserved.
//
//  Distributed under an MIT-style license; please
//  see the included LICENSE file for details.
//

#import "NSStringPunycodeAdditions.h"

// Encoding/decoding adapted/lifted from the example code in the IDNA Punycode spec (RFC 3492).
// For some other stuff, see RFC 3490 (Internationalizing Domain Names in Applications)

enum {
	base = 36,
	tmin = 1,
	tmax = 26,
	skew = 38,
	damp = 700,
	initial_bias = 72,
	initial_n = 0x80,
	delimiter = '-'
};

/* basic(cp) tests whether cp is a basic code point: */
#define basic(cp) ((unsigned)(cp) < 0x80)

/* delim(cp) tests whether cp is a delimiter: */
#define delim(cp) ((cp) == delimiter)

/* decode_digit(cp) returns the numeric value of a basic code */
/* point (for use in representing integers) in the range 0 to */
/* base-1, or base if cp is does not represent a value.       */

static NSUInteger decode_digit(unsigned cp)
{
	return  cp - 48 < 10 ? cp - 22 :  cp - 65 < 26 ? cp - 65 :
	cp - 97 < 26 ? cp - 97 : base;
}

/* encode_digit(d,flag) returns the basic code point whose value      */
/* (when used for representing integers) is d, which needs to be in   */
/* the range 0 to base-1.  The lowercase form is used unless flag is  */
/* nonzero, in which case the uppercase form is used.  The behavior   */
/* is undefined if flag is nonzero and digit d has no uppercase form. */

static char encode_digit(unsigned d, int flag)
{
	return (char)(d + 22 + 75 * (d < 26) - ((flag != 0) << 5));
	/*  0..25 map to ASCII a..z or A..Z */
	/* 26..35 map to ASCII 0..9         */
}

/* flagged(bcp) tests whether a basic code point is flagged */
/* (uppercase).  The behavior is undefined if bcp is not a  */
/* basic code point.                                        */

#define flagged(bcp) ((unsigned)(bcp) - 65 < 26)

/*** Platform-specific constants ***/

/* maxint is the maximum value of a punycode_uint variable: */
static const unsigned maxint = UINT_MAX;

/*** Bias adaptation function ***/

static NSUInteger adapt(unsigned delta, unsigned numpoints, BOOL firsttime) {
	unsigned k;
	
	delta = firsttime ? delta / damp : delta >> 1;
	delta += delta / numpoints;
	
	for (k = 0;  delta > ((base - tmin) * tmax) / 2;  k += base) {
		delta /= base - tmin;
	}
	
	return k + (base - tmin + 1) * delta / (delta + skew);
}

@interface PunycodeAddress ()

+ (NSDictionary *)URLParts:(NSString *)str;

@end

@implementation PunycodeAddress

#if BYTE_ORDER == LITTLE_ENDIAN
#define UTF32_ENCODING NSUTF32LittleEndianStringEncoding
#elif BYTE_ORDER == BIG_ENDIAN
#define UTF32_ENCODING NSUTF32BigEndianStringEncoding
#else
#error Unsupported endianness!
#endif

+ (const UTF32Char *)longCharactersWithString:(NSString *)str WithCount:(NSUInteger *)count {
	NSData *data = [str dataUsingEncoding:UTF32_ENCODING];
	*count = [data length] / sizeof(UTF32Char);
	return (const UTF32Char *)[data bytes];
}

+ (NSString *)punycodeEncodedString:(NSString *)str {
	NSMutableString *ret = [NSMutableString string];
	unsigned delta, outLen, bias, j, m, q, k, t;
	NSUInteger input_length;
	const UTF32Char *longchars = [PunycodeAddress longCharactersWithString:str WithCount:&input_length];
	
	UTF32Char n = initial_n;
	delta = outLen = 0;
	bias = initial_bias;
    
	for (j = 0;  j < input_length;  ++j) {
		if (basic(longchars[j])) {
			[ret appendFormat:@"%C", (unichar)longchars[j]];
			++outLen;
		}
	}
	
	NSUInteger b;
	NSUInteger h = b = outLen;
	
	if (b > 0)
		[ret appendFormat:@"%C", (unichar)delimiter];
	
	/* Main encoding loop: */
	
	while (h < input_length) {
		for (m = maxint, j = 0;  j < input_length;  ++j) {
			unsigned c = longchars[j];
			
			if (c >= n && c < m)
				m = longchars[j];
		}
		
		if (m - n > (maxint - delta) / (h + 1))
			return nil; //punycode_overflow;
		delta += (m - n) * (h + 1);
		n = m;
		
		for (j = 0;  j < input_length;  ++j) {
			unsigned c = longchars[j];
			
			if (c < n /* || basic([self characterAtIndex:j]) */ ) {
				if (++delta == 0)
					return nil; //punycode_overflow;
			}
			
			if (c == n) {
				for (q = delta, k = base;  ;  k += base) {
					t = k <= bias /* + tmin */ ? tmin :     /* +tmin not needed */
                    k >= bias + tmax ? tmax : k - bias;
					if (q < t)
						break;
					[ret appendFormat:@"%C", (unichar)encode_digit(t + (q - t) % (base - t), 0)];
					q = (q - t) / (base - t);
				}
				
				[ret appendFormat:@"%c", encode_digit(q, 0)];
				bias = (unsigned)adapt(delta, (unsigned)h + 1, h == b);
				delta = 0;
				++h;
			}
		}
		
		++delta, ++n;
	}
	
	return ret;
}

/*** Main decode function ***/

+ (NSString *)punycodeDecodedString:(NSString *)str {
	NSUInteger b, i, j;
	
	NSMutableData *utf32data = [NSMutableData data];
	
	/* Initialize the state: */
	NSUInteger input_length = [str length];
	UTF32Char n = initial_n;
	NSUInteger outLen = i = 0;
	NSUInteger max_out = NSUIntegerMax;
	NSUInteger bias = initial_bias;
	
	for (b = j = 0;  j < input_length;  ++j)
		if (delim([str characterAtIndex:j]))
			b = j;
	
	if (b > max_out)
		return nil; //punycode_big_output;
	
	for (j = 0;  j < b;  ++j) {
		UTF32Char c = (UTF32Char)[str characterAtIndex:j];
		
		if (!basic([str characterAtIndex:j]))
			return nil; //punycode_bad_input;
		
		[utf32data appendBytes:&c length:sizeof(c)];
		++outLen;
	}
	
	for (NSUInteger inPos = b > 0 ? b + 1 : 0; inPos < input_length; ++outLen, ++i) {
		NSUInteger k, w, t, oldi;
		
		for (oldi = i, w = 1, k = base; /* nada */ ; k += base) {
			if (inPos >= input_length)
				return nil; // punycode_bad_input;
			unsigned digit = (unsigned)decode_digit([str characterAtIndex:inPos++]);
			if (digit >= base)
				return nil; // punycode_bad_input;
			if (digit > (maxint - i) / w)
				return nil; // punycode_overflow;
			i += digit * w;
			t = k <= bias /* + tmin */ ? tmin :     /* +tmin not needed */
            k >= bias + tmax ? tmax : k - bias;
			if (digit < t)
				break;
			if (w > maxint / (base - t))
				return nil; // punycode_overflow;
			w *= (base - t);
		}
		
		bias = adapt((unsigned)i - (unsigned)oldi, (unsigned)outLen + 1, oldi == 0);
		
		if (i / (outLen + 1) > maxint - n)
			return nil; // punycode_overflow;
		n += i / (outLen + 1);
		i %= (outLen + 1);
		
		[utf32data replaceBytesInRange:NSMakeRange(i * sizeof(UTF32Char), 0) withBytes:&n length:sizeof(n)];
	}
	
#if __has_feature(objc_arc)
	return [[NSString alloc] initWithData:utf32data encoding:UTF32_ENCODING];
#else
	return [[[NSString alloc] initWithData:utf32data encoding:UTF32_ENCODING] autorelease];
#endif
}

+ (NSString *)IDNAEncodedString:(NSString *)str {
	NSCharacterSet *nonAscii = [[NSCharacterSet characterSetWithRange:NSMakeRange(1, 127)] invertedSet];
	NSMutableString *ret = [NSMutableString string];
	NSScanner *s = [NSScanner scannerWithString:[str precomposedStringWithCompatibilityMapping]];
	NSCharacterSet *dotAt = [NSCharacterSet characterSetWithCharactersInString:@".@"];
	NSString *input = nil;
	
	while (![s isAtEnd]) {
		if ([s scanUpToCharactersFromSet:dotAt intoString:&input]) {
			if ([input rangeOfCharacterFromSet:nonAscii].location != NSNotFound) {
				[ret appendFormat:@"xn--%@", [PunycodeAddress punycodeEncodedString:input]];
			} else
				[ret appendString:input];
		}
		
		if ([s scanCharactersFromSet:dotAt intoString:&input])
			[ret appendString:input];
	}
    
	return ret;
}

+ (NSString *)IDNADecodedString:(NSString *)str {
	NSMutableString *ret = [NSMutableString string];
	NSScanner *s = [NSScanner scannerWithString:str];
	NSCharacterSet *dotAt = [NSCharacterSet characterSetWithCharactersInString:@".@"];
	NSString *input = nil;
	
	while (![s isAtEnd]) {
		if ([s scanUpToCharactersFromSet:dotAt intoString:&input]) {
			if ([[input lowercaseString] hasPrefix:@"xn--"]) {
				NSString *substr = [PunycodeAddress punycodeDecodedString:[input substringFromIndex:4]];
				
				if (substr)
					[ret appendString:substr];
			} else
				[ret appendString:input];
		}
		
		if ([s scanCharactersFromSet:dotAt intoString:&input])
			[ret appendString:input];
	}
	
	return ret;
}

+ (NSDictionary *)URLParts:(NSString *)str {
	NSCharacterSet *colonSlash = [NSCharacterSet characterSetWithCharactersInString:@":/"];
	NSScanner *s = [NSScanner scannerWithString:[str precomposedStringWithCompatibilityMapping]];
	NSString *scheme = @"";
	NSString *delim = @"";
	NSString *username = nil;
	NSString *password = nil;
	NSString *host = @"";
	NSString *path = @"";
	NSString *fragment = nil;
	
	if ([s scanUpToCharactersFromSet:colonSlash intoString:&host]) {
		if (![s isAtEnd] && [str characterAtIndex:[s scanLocation]] == ':') {
			scheme = host;
			
			if (![s isAtEnd])
				[s scanCharactersFromSet:colonSlash intoString:&delim];
			if (![s isAtEnd])
				[s scanUpToCharactersFromSet:colonSlash intoString:&host];
		}
	}
	
	if (![s isAtEnd])
		[s scanUpToString:@"#" intoString:&path];
    
	if (![s isAtEnd]) {
		[s scanString:@"#" intoString:nil];
		[s scanUpToCharactersFromSet:[NSCharacterSet newlineCharacterSet] intoString:&fragment];
	}
	
	NSCharacterSet *colonAt = [NSCharacterSet characterSetWithCharactersInString:@":@"];
	
	s = [NSScanner scannerWithString:host];
	NSString *temp = nil;
	
	if ([s scanUpToCharactersFromSet:colonAt intoString:&temp]) {
		if (![s isAtEnd]) {
			username = temp;
			
			if ([host characterAtIndex:[s scanLocation]] == ':') {
				[s scanCharactersFromSet:colonAt intoString:&temp];
                
				if (![s isAtEnd] && [s scanUpToCharactersFromSet:colonAt intoString:&temp])
					password = temp;
			}
			
			[s scanCharactersFromSet:colonAt intoString:nil];
            
			if (![s isAtEnd] && [s scanUpToCharactersFromSet:colonAt intoString:&temp])
				host = temp;
		}
	}
	
	NSMutableDictionary *parts = [NSMutableDictionary dictionaryWithObjectsAndKeys:
								  scheme,	@"scheme",
								  delim,	@"delim",
								  host,		@"host",
								  path,		@"path",
								  nil];
	
	if (username)
		[parts setObject:username forKey:@"username"];
	if (password)
		[parts setObject:password forKey:@"password"];
	if (fragment)
		[parts setObject:fragment forKey:@"fragment"];
	
	return parts;
}

+ (NSString *)decodedString:(NSString *)str {
	NSDictionary *urlParts = [PunycodeAddress URLParts:str];
	
	NSString *ret = [NSString stringWithFormat:@"%@%@%@%@", [urlParts objectForKey:@"scheme"], [urlParts objectForKey:@"delim"], [PunycodeAddress IDNADecodedString:[urlParts objectForKey:@"host"]], [[urlParts objectForKey:@"path"] stringByRemovingPercentEncoding]];
	
	if ([urlParts objectForKey:@"fragment"])
		ret = [ret stringByAppendingFormat:@"#%@", [urlParts objectForKey:@"fragment"]];
	
	return ret;
}

+ (NSString *)encodedString:(NSString *)str {
	// We can't get the parts of an URL for an international domain name, so a custom method is used instead.
	NSDictionary *urlParts = [PunycodeAddress URLParts:str];
	NSString *path = [urlParts objectForKey:@"path"];
    path = [path stringByAddingPercentEncodingWithAllowedCharacters:[NSCharacterSet URLPathAllowedCharacterSet]];
	
#if !__has_feature(objc_arc)
	[path autorelease];
#endif
	
	NSMutableString *ret = [NSMutableString stringWithFormat:@"%@%@", [urlParts objectForKey:@"scheme"], [urlParts objectForKey:@"delim"]];
	if ([urlParts objectForKey:@"username"]) {
		if ([urlParts objectForKey:@"password"])
			[ret appendFormat:@"%@:%@@", [urlParts objectForKey:@"username"], [urlParts objectForKey:@"password"]];
		else
			[ret appendFormat:@"%@@", [urlParts objectForKey:@"username"]];
	}
	
	[ret appendFormat:@"%@%@", [PunycodeAddress IDNAEncodedString:[urlParts objectForKey:@"host"]], path];
	
	if ([urlParts objectForKey:@"fragment"])
		[ret appendFormat:@"#%@", [urlParts objectForKey:@"fragment"]];
    
	return ret;
}

+ (NSURL *)URLWithUnicodeString:(NSString *)URLString {
	return [NSURL URLWithString:[PunycodeAddress encodedString:URLString]];
}

+ (NSString *)decodedURLString:(NSURL *)url {
	return [PunycodeAddress decodedString:[url absoluteString]];
}

@end
