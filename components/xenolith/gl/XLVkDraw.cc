/**
 Copyright (c) 2020 Roman Katuntsev <sbkarr@stappler.org>

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

#include "XLVkDraw.h"
#include "XLVkFramebuffer.h"

#include "XLVkDefaultShader.cc"

namespace stappler::xenolith::vk {

class DrawDevice::Worker : public Ref {
public:
	~Worker();

	bool init(DrawDevice *draws, size_t nImages, uint32_t queue, uint32_t index);

	uint32_t getIndex() const { return _index; }

	void reset(uint32_t);
	VkCommandBuffer spawnPool(uint32_t);

protected:
	uint32_t _index = maxOf<uint32_t>();
	DrawDevice *_draws = nullptr;
	Vector<Rc<CommandPool>> _pools;
};


DrawDevice::Worker::~Worker() {
	for (Rc<CommandPool> &it : _pools) {
		it->invalidate(*_draws);
	}
	_pools.clear();
}

bool DrawDevice::Worker::init(DrawDevice *draws, size_t nImages, uint32_t queue, uint32_t index) {
	_draws = draws;
	_index = index;
	_pools.reserve(nImages);
	for (size_t i = 0; i < nImages; ++ i) {
		auto p = Rc<CommandPool>::create(*_draws, queue);
		if (!p) {
			return false;
		}
		_pools.emplace_back(p);
	}

	return true;
}

void DrawDevice::Worker::reset(uint32_t image) {
	_pools[image]->reset(*_draws, false);
}

VkCommandBuffer DrawDevice::Worker::spawnPool(uint32_t image) {
	return _pools[image]->allocBuffer(*_draws);
}

DrawDevice::~DrawDevice() {
	if (_defaultPipelineLayout) {
		_defaultPipelineLayout->invalidate(*this);
	}
	for (auto &it : _pipelineLayouts) {
		it.second->invalidate(*this);
	}
	_pipelineLayouts.clear();

	_workers.clear();

	for (auto shader : _shaders) {
		shader.second->invalidate(*this);
	}
	_shaders.clear();
}

bool DrawDevice::init(Rc<Instance> inst, Rc<Allocator> alloc, VkQueue q, uint32_t qIdx) {
	if (!VirtualDevice::init(inst, alloc)) {
		return false;
	}

	_queue = q;
	_queueIdx = qIdx;

	auto vert = Rc<ProgramModule>::create(*this, ProgramSource::Glsl, ProgramStage::Vertex, defaultVert, "default.vert");
	auto frag = Rc<ProgramModule>::create(*this, ProgramSource::Glsl, ProgramStage::Fragment, defaultFrag, "default.frag");

	_defaultPipelineLayout = Rc<PipelineLayout>::create(*this);

	_shaders.emplace(vert->getName().str(), vert);
	_shaders.emplace(frag->getName().str(), frag);

	_frameTasks.emplace_back(BufferTask(BufferTask::HardTask, [&] (const FrameInfo &frame, VkCommandBuffer buf) -> bool {
		VkCommandBufferBeginInfo beginInfo { };
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		if (_instance->vkBeginCommandBuffer(buf, &beginInfo) != VK_SUCCESS) {
			return false;
		}

		VkRenderPassBeginInfo renderPassInfo { };
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _defaultRenderPass->getRenderPass();
		renderPassInfo.framebuffer = frame.framebuffer->getFramebuffer();
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = frame.options->capabilities.currentExtent;
		VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;
		_instance->vkCmdBeginRenderPass(buf, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		_instance->vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, _defaultPipeline->getPipeline());
		_instance->vkCmdDraw(buf, 3, 1, 0, 0);
		_instance->vkCmdEndRenderPass(buf);
		if (_instance->vkEndCommandBuffer(buf) != VK_SUCCESS) {
			return false;
		}

		return true;
	}));

	return true;
}

bool DrawDevice::spawnWorkers(thread::TaskQueue &q, size_t nImages, const Instance::PresentationOptions &opts, VkFormat format) {
	_nImages = nImages;

	auto vert = _shaders.at("default.vert");
	auto frag = _shaders.at("default.frag");

	_defaultRenderPass = Rc<RenderPass>::create(*this, format);
	if (!_defaultRenderPass) {
		return false;
	}

	_defaultPipeline = Rc<Pipeline>::create(*this,
		Pipeline::Options({_defaultPipelineLayout->getPipelineLayout(), _defaultRenderPass->getRenderPass(), {
			pair(vert->getStage(), vert->getModule()), pair(frag->getStage(), frag->getModule())
		}}),
		GraphicsParams({
			Rect(0.0f, 0.0f, opts.capabilities.currentExtent.width, opts.capabilities.currentExtent.height),
			GraphicsParams::URect{0, 0, opts.capabilities.currentExtent.width, opts.capabilities.currentExtent.height}
		}));


	uint32_t idx = std::min(uint32_t(q.getThreadsCount() / 4), uint32_t(1));
	Vector<uint32_t> indexes;

	size_t i = 0;
	for (size_t j = 0; j < idx; ++ j) {
		indexes.emplace_back(0);
		++ i;
	}
	for (size_t j = 0; j < idx; ++ j) {
		indexes.emplace_back(1);
		++ i;
	}
	while (i < q.getThreadsCount()) {
		indexes.emplace_back(2);
		++ i;
	}

	_defaultWorker = Rc<Worker>::create(this, _nImages, _queueIdx, 0);

	Map<uint32_t, Vector<Rc<Task>>> tasks;
	for (size_t i = 0; i < q.getThreadsCount(); ++ i) {
		tasks.emplace(i, Vector<Rc<thread::Task>>({
			Rc<thread::Task>::create([this, &indexes, i] (const thread::Task &) -> bool {
				std::unique_lock<std::mutex> lock(_mutex);
				auto it = _workers.find(std::this_thread::get_id());
				if (it == _workers.end()) {
					auto b = indexes.back();
					indexes.pop_back();
					_workers.emplace(std::this_thread::get_id(), Rc<Worker>::create(this, _nImages, _queueIdx, b));
					std::cout.flush();
				}
				return true;
			}, [this] (const thread::Task &, bool success) { })
		}));
	}

	q.perform(move(tasks));
	q.waitForAll(TimeInterval::microseconds(2000));

	return true;
}

void DrawDevice::invalidate() {
	_defaultWorker = nullptr;
	_defaultPipeline->invalidate(*this);
	_defaultRenderPass->invalidate(*this);

	_workers.clear();
}

Vector<VkCommandBuffer> DrawDevice::fillBuffers(thread::TaskQueue &q, FrameInfo &frame) {
	Vector<VkCommandBuffer> ret;

	Vector<BufferTask> tasks(_frameTasks);
	std::sort(_frameTasks.begin(), _frameTasks.end(), [&] (const BufferTask &l, const BufferTask &r) {
		return l.index < r.index;
	});

	// std::cout << frame.imageIdx << "\n";

	/*_defaultWorker->reset(frame.imageIdx);
	while (auto t = getTaskForWorker(tasks, _defaultWorker)) {
		auto buf = _defaultWorker->spawnPool(frame.imageIdx);
		if (t.callback(frame, buf)) {
			ret.emplace_back(buf);
		}
	}*/

	Map<uint32_t, Vector<Rc<Task>>> threadTasks;
	for (size_t i = 0; i < q.getThreadsCount(); ++ i) {
		Vector<VkCommandBuffer> * cmds = new Vector<VkCommandBuffer>;

		threadTasks.emplace(i, Vector<Rc<thread::Task>>({
			Rc<thread::Task>::create([this, &tasks, cmds, &frame] (const thread::Task &) -> bool {
				if (Worker * w = _workers[std::this_thread::get_id()]) {
					w->reset(frame.imageIdx);
					while (auto t = getTaskForWorker(tasks, w)) {
						auto buf = w->spawnPool(frame.imageIdx);
						if (t.callback(frame, buf)) {
							cmds->emplace_back(buf);
						}
					}
				}
				return true;
			}, [this, cmds, &ret] (const thread::Task &, bool success) {
				for (VkCommandBuffer it : *cmds) {
					ret.emplace_back(it);
				}
				delete cmds;
			})
		}));
	}

	q.perform(move(threadTasks));
	q.waitForAll(TimeInterval::microseconds(2000));

	return ret;
}

bool DrawDevice::drawFrame(thread::TaskQueue &q, FrameInfo &frame) {
	auto bufs = fillBuffers(q, frame);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	submitInfo.waitSemaphoreCount = frame.wait.size();
	submitInfo.pWaitSemaphores = frame.wait.data();
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.commandBufferCount = bufs.size();
	submitInfo.pCommandBuffers = bufs.data();
	submitInfo.signalSemaphoreCount = frame.signal.size();
	submitInfo.pSignalSemaphores = frame.signal.data();

	if (_instance->vkQueueSubmit(_queue, 1, &submitInfo, frame.fence) != VK_SUCCESS) {
		stappler::log::vtext("VK-Error", "Fail to vkQueueSubmit");
		return false;
	}

	return true;
}

DrawDevice::BufferTask DrawDevice::getTaskForWorker(Vector<BufferTask> &tasks, Worker *worker) {
	if (tasks.empty()) {
		return BufferTask({0, nullptr});
	}

	std::unique_lock<std::mutex> lock(_mutex);
	auto it = std::lower_bound(tasks.begin(), tasks.end(), worker->getIndex(), [&] (const BufferTask &t, uint32_t idx) {
		return t.index < idx;
	});

	if (it != tasks.end()) {
		auto tmp = *it;
		tasks.erase(it);
		return tmp;
	}

	return BufferTask({0, nullptr});
}

}
