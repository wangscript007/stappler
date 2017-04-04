/**
Copyright (c) 2016-2017 Roman Katuntsev <sbkarr@stappler.org>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
**/

#ifndef LAYOUT_VG_SLCANVAS_H_
#define LAYOUT_VG_SLCANVAS_H_

#include "SLPath.h"
#include "SLTesselator.h"
#include "SLMemPool.h"

NS_LAYOUT_BEGIN

class Canvas : public Ref {
public:
	/* Approximation quality defines how percisely original curves will be approximated with lines
	 * Extreme values (Worst/Perfect) should be used only for special cases
	 * In most cases Low is enought to draw simple vector objects (like material icons)
	 *
	 * In Stappler, PathNode uses High quality, renderer for material icons uses Low
	 */

	constexpr static float QualityWorst = 0.25f;
	constexpr static float QualityLow = 0.75f;
	constexpr static float QualityNormal = 1.25f;
	constexpr static float QualityHigh = 1.75f;
	constexpr static float QualityPerfect = 2.25f;

	virtual bool init();

	virtual void flush();

	void setQuality(float value);
	float getQuality() const;

	void beginBatch();
	void endBatch();

	void draw(const Path &);
	void draw(const Path &, const Mat4 &, bool force = false);
	void draw(const Path &, float tx, float ty);

	void scale(float sx, float sy);
	void translate(float tx, float ty);
	void transform(const Mat4 &);

	void save();
	void restore();

	void setLineWidth(float);

	void pathBegin(const Path &);
	void pathEnd(const Path &);

	void pathMoveTo(const Path &, float x, float y);
	void pathLineTo(const Path &, float x, float y);
	void pathQuadTo(const Path &, float x1, float y1, float x2, float y2);
	void pathCubicTo(const Path &, float x1, float y1, float x2, float y2, float x3, float y3);
	void pathArcTo(const Path &, float rx, float ry, float angle, bool largeArc, bool sweep, float x, float y);
	void pathClose(const Path &);

protected:
	void initPath(const Path &);
	void finalizePath(const Path &);

	void pushContour(const Path &, bool closed);
	void clearTess();

	TESSalloc _tessAlloc;

	TESStesselator *_fillTess = nullptr;
	Vector<TESStesselator *> _tess;
	Vector<StrokeDrawer> _stroke;
	MemPool<false> _pool;

	Mat4 _transform;
	Vector<Mat4> _states;
	Vector<Mat4> _batchStates;
	LineDrawer _line;

	size_t _vertexCount = 0;
	uint32_t _width = 0;
	uint32_t _height = 0;
	bool _isBatch = false;
	float _lineWidth = 1.0f;
	float _approxScale = 1.0f;
	float _quality = 0.5f; // approximation level (more is better)
	float _pathX = 0.0f;
	float _pathY = 0.0f;

	TimeInterval _subAccum;
	DrawStyle _pathStyle = DrawStyle::None;
};

NS_LAYOUT_END

#endif /* LAYOUT_VG_SLCANVAS_H_ */