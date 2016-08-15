
#ifndef StapplerPress_AcceleratedAction_h
#define StapplerPress_AcceleratedAction_h

#include "SPDefine.h"
#include "2d/CCActionInterval.h"

NS_SP_BEGIN

class Accelerated : public cocos2d::ActionInterval {
public:
	/** Движение от начальной точки до указанной, действие завершается по достижении конечной точки */
	static ActionInterval *createBounce(float acceleration, const Vec2 &from, const Vec2 &to, const Vec2 &velocity = Vec2::ZERO, float bounceAcceleration = 0, std::function<void(cocos2d::Node *)> callback = nullptr);

	/** Движение от начальной точки до указанной, действие завершается по достижении конечной точки */
	static ActionInterval *createBounce(float acceleration, const Vec2 &from, const Vec2 &to, float velocity, float bounceAcceleration = 0, std::function<void(cocos2d::Node *)> callback = nullptr);

	/** Движение от начальной точки до указанной, действие завершается при сбросе скорости до 0 */
	static ActionInterval *createFreeBounce(float acceleration, const Vec2 &from, const Vec2 &to, const Vec2 &velocity = Vec2::ZERO, float bounceAcceleration = 0, std::function<void(cocos2d::Node *)> callback = nullptr);

	/** Движение от начальной точки в направлении вектора скорости, действие завершается при границ */
	static ActionInterval *createWithBounds(float acceleration, const Vec2 &from, const Vec2 &velocity, const Rect &bounds, std::function<void(cocos2d::Node *)> callback = nullptr);

	/** отрезок пути до полной остановки (скорость и ускорение направлены в разные стороны) */
	static Accelerated *createDecceleration(const Vec2 & normal, const Vec2 & startPoint, float startVelocity, float acceleration);
	virtual bool initDecceleration(const Vec2 & normal, const Vec2 & startPoint, float startVelocity, float acceleration);


	/** отрезок пути до полной остановки (скорость и ускорение направлены в разные стороны) */
	static Accelerated *createDecceleration(const Vec2 & startPoint, const Vec2 & endPoint, float acceleration);
	virtual bool initDecceleration(const Vec2 & startPoint, const Vec2 & endPoint, float acceleration);


	/** ускорение до достижения конечной скорости */
	static Accelerated *createAccelerationTo(const Vec2 &normal,const Vec2 & startPoint, float startVelocity, float endVelocity, float acceleration);
	virtual bool initAccelerationTo(const Vec2 & normal, const Vec2 & startPoint, float startVelocity, float endVelocity,float acceleration);


	/** ускорение до достижения конечной точки */
	static Accelerated *createAccelerationTo(const Vec2 & startPoint, const Vec2 & endPoint, float startVelocity, float acceleration);
	virtual bool initAccelerationTo(const Vec2 & startPoint, const Vec2 & endPoint, float startVelocity, float acceleration);


	/** ускоренное движение по времени */
	static Accelerated *createWithDuration(float duration, const Vec2 &normal, const Vec2 &startPoint, float startVelocity, float acceleration);
	virtual bool initWithDuration(float duration, const Vec2 &normal, const Vec2 &startPoint, float startVelocity, float acceleration);

	float getDuration() const;

	Vec2 getPosition(float timePercent) const;

	const Vec2 &getStartPosition() const;
	const Vec2 &getEndPosition() const;
	const Vec2 &getNormal() const;

	float getStartVelocity() const;
	float getEndVelocity() const;
	float getCurrentVelocity() const;

    virtual void startWithTarget(cocos2d::Node *target) override;
	virtual ActionInterval* clone() const override;
    virtual ActionInterval* reverse() const override;
    virtual void update(float time) override;

	virtual void setCallback(std::function<void(cocos2d::Node *)> callback);

protected:
	float _duration;

	float _acceleration;

	float _startVelocity;
	float _endVelocity;

	Vec2 _normalPoint;
	Vec2 _startPoint;
	Vec2 _endPoint;

	Vec2 computeEndPoint();
	Vec2 computeNormalPoint();
	float computeEndVelocity();

	std::function<void(cocos2d::Node *)> _callback = nullptr;
};

NS_SP_END

#endif
