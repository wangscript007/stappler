# Кодирование URL

{{TOC}}

## Стандартный формат

Serenity поддерживает стандартный формат кодирования URL с допонениями (подобно PHP). Таким образом, формат позволяет определять не только пары ключ/значение, но и последовательные и ассоциативные массивы:

Определение ассоциативного массива:

```
https://example.com/path?dict[dictKey]=dictValue&dict[anotherKey]=anotherValue

// Будет передано на вход как
{
   "dict": {
     "dictKey": "dictValue",
     "anotherKey" : "anotherValue"
   }
}
```

Определение последовательного массива:

```
https://example.com/path?arr[]=value1&arr[]=value2

// Будет передано на вход как
{
   "arr": [
     "value1",
     "value2"
   ]
}
```

## Формат Serenity

Специальный формат служит для определения сложных структур в строке URL. Значимая единица кодирования - токен - символьная строка из допустиммых символов.

Допустимые символы в токенах:

```
0-9 A-Z a-z / ? @ - . _ ! $ ' * +
```

Допустимые специальные символы в строке запроса (из RFC): 

```
/ ? : @ - . _ ~ ! $ & ' ( ) * + , ; =
```

Запрещены в токенах:

```
: & ( ) , ; = ~ #
```

Другие символы можно кодировать с помощью традиционного процентного кодирования. Кроме того, процентное кодирование позволяет использовть имена, совпадающие с зарезервированными токенами:

```
null true false inf +inf -inf nan
```

Пример кодирования:

```
http://localhost/index?(field:value;arrayField:array1,array2,array3;flagField;dictField(field1:value1;field2:value2))

// Форматированно
(
	field: value;
	arrayField: array1, array2, array3;
	flagField;
	dictField (
		field1: value1;
		field2: value2
	)
)

// В JSON
{
   “field”: “value”,
   “arrayField”: [“array1”, ”array2”, ”array3”],
   “flagField”: true,
   “dictField”: {
       “field1”: “value1”,
       “field2”: “value2”
   }
}
```

Кодирование начинается с символа `?` с последующим выражением, обёрнутым в `( )`. Внутри объекта в скобках находятся пары ключ/значение или флаговые ключи, разделённые `;`. Значение может интерпретироваться как число, строка или массив. Число это строка состоящая из цифр и спецсимволов `. + -`, массив определяется по разделителю `,`.

```
(
	string:string;
	array:array1,array2,array3;
	arrayWithinArray:value1,~(subvalue1,subvalue2),value3
	dict(field1:value1;field2:value2)
	flag;
	number:123
)
```

Кодирование влоенных структур:

```
// Кодирование вложенных полей Serenity
(fields:$defaults,content,terms,images($all)documents(title,content,images($all)))

// В этом случае для вложенных объектов image раскрываются все поля ( images($all) )
// Для documents раскрываются поля title, content и объекты в images

```
