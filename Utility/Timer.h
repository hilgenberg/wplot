#pragma once

double now(); ///< Must be in seconds since some reference date. Higher resolution is better.

void sleep(double dt);

class Timer
{
public:
	Timer(double dt);
	~Timer();
	void start();
	void stop();
	bool running() const;
	void run(bool f) { if (running() && !f) stop(); else if (!running() && f) start(); }
	double dt() const { return dt_; }
	std::function<void(void)> callback;

private:
	#ifdef _WINDOWS
	HANDLE timer;
	#endif
	double dt_;
};
