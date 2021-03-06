// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/**
Copyright (c) 2016 Roman Katuntsev <sbkarr@stappler.org>

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

#include "SPTime.h"
#include "SPDefine.h"
#include "SPPlatform.h"

#include "SPJNI.h"
#include "SPThreadManager.h"

#if (ANDROID)

#include "platform/CCFileUtils.h"

#define MAIN_LOOP_FRAME_TIME 16000
#define MAIN_LOOP_FRAME_PAUSE 500000
#define MAIN_LOOP_FRAME_RESUME 100000

NS_SP_BEGIN

class MainLoop {
public:
	MainLoop();
	~MainLoop();

    bool init();
    bool update();

    void framePerformed();
    void requestRender();
    void finalize();

    void start();
    void stop();

    void blockRendering();
    void unblockRendering();

    bool setRenderWhenDirty();
    bool setRenderContinuously();

protected:
    void render();

	std::atomic_flag _shouldQuit;
	std::atomic_flag _renderFlag;
	std::atomic_flag _frameFlag;

	std::atomic<bool> _running;
	std::atomic<bool> _blocked;

	std::thread _thread;

	uint64_t _mksec = 0;
	uint64_t _stopTimer = 0;

	bool _renderContinuously = false;
	jmethodID _renderFrame = nullptr;
	jmethodID _setRenderContinuously = nullptr;
};

void doMainLoop(MainLoop *loop);

MainLoop::MainLoop() : _thread(doMainLoop, this) {
	_shouldQuit.test_and_set();
	_running.store(true);
	_blocked.store(false);
}

MainLoop::~MainLoop() {

}

bool MainLoop::init() {
	_shouldQuit.test_and_set();
	_renderFlag.test_and_set();
	_frameFlag.clear();

	_mksec = Time::now().toMicroseconds();

	JNIEnv *env = stappler::spjni::getJniEnv();
	auto & activity = stappler::spjni::getActivity(env);
	if (activity) {
		auto activityClass = activity.get_class();
		_renderFrame = spjni::getMethodID(env, activityClass, "renderFrame", "()V");
		_setRenderContinuously = spjni::getMethodID(env, activityClass, "setRenderContinuously", "(Z)V");
		return _renderFrame && _setRenderContinuously;
	} else {
		return false;
	}
}

void MainLoop::render() {
	JNIEnv *env = stappler::spjni::getJniEnv();
	auto &activity = stappler::spjni::getActivity(env);
	env->CallVoidMethod(activity, _renderFrame);
}

bool MainLoop::update() {
	if (!_shouldQuit.test_and_set()) {
		return false;
	}

	if (!_running.load()) {
		return true;
	}

    if (!_renderFrame || !_setRenderContinuously) {
        if (!init()) {
            return true;
        }
    }

	auto now = Time::now().toMicroseconds();
	auto diff = now - _mksec;

	bool renderRequested = false;
	if (!_renderFlag.test_and_set()) {
		renderRequested = true;
		_stopTimer = MAIN_LOOP_FRAME_RESUME;
		setRenderContinuously();
		diff = 0;
	}

	if (_renderContinuously) {
		if (_stopTimer > diff) {
			_stopTimer -= diff;
		} else {
			_stopTimer = MAIN_LOOP_FRAME_RESUME;
			_mksec = now;
			setRenderWhenDirty();
		}
	} else {
		if (renderRequested || diff > MAIN_LOOP_FRAME_PAUSE || _stopTimer > diff) {
			if (!_frameFlag.test_and_set()) {
				_mksec = now;
				render();

				if (_stopTimer > diff) {
					_stopTimer -= diff;
				} else {
					_stopTimer = 0;
				}
			}
		}
	}

	return true;
}

void MainLoop::framePerformed() {
	_frameFlag.clear();
}

void MainLoop::requestRender() {
	_renderFlag.clear();
}

void MainLoop::finalize() {
	_shouldQuit.clear();
}

void MainLoop::start() {
	_running.store(true);
}

void MainLoop::stop() {
	_running.store(false);
}

void MainLoop::blockRendering() {
	_blocked.store(true);
}

void MainLoop::unblockRendering() {
	_blocked.store(false);
}

bool MainLoop::setRenderWhenDirty() {
	if (!_blocked.load()) {
		JNIEnv *env = stappler::spjni::getJniEnv();
		auto & activity = stappler::spjni::getActivity(env);
		env->CallVoidMethod(activity, _setRenderContinuously, false);
		_renderContinuously = false;
		return true;
	}
	return false;
}

bool MainLoop::setRenderContinuously() {
	JNIEnv *env = stappler::spjni::getJniEnv();
	auto & activity = stappler::spjni::getActivity(env);
	env->CallVoidMethod(activity, _setRenderContinuously, true);
	_renderContinuously = true;
	return true;
}

void doMainLoop(MainLoop *loop) {
	JavaVM *vm = cocos2d::JniHelper::getJavaVM();
	JNIEnv *env = NULL;
	vm->AttachCurrentThread(&env, NULL);

	usleep(MAIN_LOOP_FRAME_TIME);
	loop->init();
	while(loop->update()) {
		usleep(MAIN_LOOP_FRAME_TIME);
	}

    vm->DetachCurrentThread();
    delete loop;
}


NS_SP_END

namespace stappler::platform::render {
	MainLoop *_loop = nullptr;

	void _init() {
		if (!_loop) {
			_loop = new MainLoop();
		} else {
			_loop->start();
		}
	}
	void _requestRender() {
		if (_loop) {
			_loop->requestRender();
		}
	}
	void _framePerformed() {
		if (_loop) {
			_loop->framePerformed();
		}
	}

	void _blockRendering() {
		if (_loop) {
			_loop->blockRendering();
		}
	}

	void _unblockRendering() {
		if (_loop) {
			_loop->unblockRendering();
		}
	}

	void _stop() {
		if (_loop) {
			_loop->stop();
		}
	}
	void _start() {
		if (_loop) {
			_loop->start();
		}
	}

	bool _enableOffscreenContext() {
		if (!ThreadManager::getInstance()->isMainThread()) {
			JNIEnv *env = stappler::spjni::getJniEnv();
			auto & activity = stappler::spjni::getActivity(env);
			auto activityClass = activity.get_class();
			auto enableOffscreenContext = spjni::getMethodID(env, activityClass, "enableOffscreenContext", "()V");
			env->CallVoidMethod(activity, enableOffscreenContext);
			return true;
		}
		return false;
	}
	void _disableOffscreenContext() {
		JNIEnv *env = stappler::spjni::getJniEnv();
		auto & activity = stappler::spjni::getActivity(env);
		auto activityClass = activity.get_class();
		auto disableOffscreenContext = spjni::getMethodID(env, activityClass, "disableOffscreenContext", "()V");
		env->CallVoidMethod(activity, disableOffscreenContext);
	}
}

#endif
