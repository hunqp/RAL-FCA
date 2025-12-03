/********************************************************************
 * Copyright (C) 2015 Liangliang Nan <liangliang.nan@gmail.com>
 * https://3d.bk.tudelft.nl/liangliang/
 *
 * This file is part of Easy3D. If it is useful in your research/work,
 * I would be grateful if you show your appreciation by citing it:
 * ------------------------------------------------------------------
 *      Liangliang Nan.
 *      Easy3D: a lightweight, easy-to-use, and efficient C++ library
 *      for processing and rendering 3D data.
 *      Journal of Open Source Software, 6(64), 3255, 2021.
 * ------------------------------------------------------------------
 *
 * Easy3D is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License Version 3
 * as published by the Free Software Foundation.
 *
 * Easy3D is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ********************************************************************/

#ifndef EASY3D_UTIL_TIMER_H
#define EASY3D_UTIL_TIMER_H

#include <thread>
#include <chrono>
#include <functional>

namespace easy3d {

	template<class... Args>
	class Timer {
	public:
		Timer() : stopped_(false), paused_(false) {}

		~Timer() {}

		static void single_shot(int delay, std::function<void(Args...)> const &func, Args... args);

		template<class Class>
		static void single_shot(int delay, Class *inst, void (Class::*func)(Args...), Args... args);

		template<class Class>
		static void single_shot(int delay, Class const *inst, void (Class::*func)(Args...) const, Args... args);

		void set_timeout(int delay, std::function<void(Args...)> const &func, Args... args) const;

		template<class Class>
		void set_timeout(int delay, Class *inst, void (Class::*func)(Args...), Args... args) const;

		template<class Class>
		void set_timeout(int delay, Class const *inst, void (Class::*func)(Args...) const, Args... args) const;

		void set_interval(int interval, std::function<void(Args...)> const &func, Args... args) const;

		template<class Class>
		void set_interval(int interval, Class *inst, void (Class::*func)(Args...), Args... args) const;

		template<class Class>
		void set_interval(int interval, Class const *inst, void (Class::*func)(Args...) const, Args... args) const;

		void stop() {
			stopped_ = true;
		}

		bool is_stopped() const {
			return stopped_;
		}

		void pause() {
			paused_ = true;
		}

		bool is_paused() const {
			return paused_;
		}

		void resume() {
			if (!stopped_ && paused_)
				paused_ = false;
		}

	private:
		bool stopped_;
		bool paused_;
	};

	//-------------------------- IMPLEMENTATION ---------------------------

	template<class... Args>
	void Timer<Args...>::single_shot(int delay, std::function<void(Args...)> const &func, Args... args) {
		std::thread t([=]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			func(args...);
		});
		t.detach();
	}

	template<class... Args>
	template<class Class>
	void Timer<Args...>::single_shot(int delay, Class *inst, void (Class::*func)(Args...), Args... args) {
		std::thread t([=]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			(inst->*func)(args...);
		});
		t.detach();
	}

	template<class... Args>
	template<class Class>
	void Timer<Args...>::single_shot(int delay, Class const *inst, void (Class::*func)(Args...) const, Args... args) {
		std::thread t([=]() {
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			(inst->*func)(args...);
		});
		t.detach();
	}

	template<class... Args>
	void Timer<Args...>::set_timeout(int delay, std::function<void(Args...)> const &func, Args... args) const {
		const_cast<Timer<Args...> *>(this)->stopped_ = false;
		std::thread t([=]() {
			if (stopped_)
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			if (stopped_)
				return;
			func(args...);
		});
		t.detach();
	}

	template<class... Args>
	template<class Class>
	void Timer<Args...>::set_timeout(int delay, Class *inst, void (Class::*func)(Args...), Args... args) const {
		const_cast<Timer<Args...> *>(this)->stopped_ = false;
		std::thread t([=]() {
			if (stopped_)
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			if (stopped_)
				return;
			(inst->*func)(args...);
		});
		t.detach();
	}

	template<class... Args>
	template<class Class>
	void Timer<Args...>::set_timeout(int delay, Class const *inst, void (Class::*func)(Args...) const, Args... args) const {
		const_cast<Timer<Args...> *>(this)->stopped_ = false;
		std::thread t([=]() {
			if (stopped_)
				return;
			std::this_thread::sleep_for(std::chrono::milliseconds(delay));
			if (stopped_)
				return;
			(inst->*func)(args...);
		});
		t.detach();
	}

	template<class... Args>
	void Timer<Args...>::set_interval(int interval, std::function<void(Args...)> const &func, Args... args) const {
		const_cast<Timer<Args...> *>(this)->stopped_ = false;
		std::thread t([=]() {
			while (true) {
				if (stopped_)
					return;
				std::this_thread::sleep_for(std::chrono::milliseconds(interval));
				if (paused_)
					continue;
				else if (stopped_)
					return;
				func(args...);
			}
		});
		t.detach();
	}

	template<class... Args>
	template<class Class>
	void Timer<Args...>::set_interval(int interval, Class *inst, void (Class::*func)(Args...), Args... args) const {
		const_cast<Timer<Args...> *>(this)->stopped_ = false;
		std::thread t([=]() {
			while (true) {
				if (stopped_)
					return;
				std::this_thread::sleep_for(std::chrono::milliseconds(interval));
				if (paused_)
					continue;
				else if (stopped_)
					return;
				(inst->*func)(args...);
			}
		});
		t.detach();
	}

	template<class... Args>
	template<class Class>
	void Timer<Args...>::set_interval(int interval, Class const *inst, void (Class::*func)(Args...) const, Args... args) const {
		const_cast<Timer<Args...> *>(this)->stopped_ = false;
		std::thread t([=]() {
			while (true) {
				if (stopped_)
					return;
				std::this_thread::sleep_for(std::chrono::milliseconds(interval));
				if (paused_)
					continue;
				else if (stopped_)
					return;
				(inst->*func)(args...);
			}
		});
		t.detach();
	}

}	 // namespace easy3d

#endif	  // EASY3D_UTIL_TIMER_H