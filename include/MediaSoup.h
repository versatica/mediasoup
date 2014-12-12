#ifndef MS_MEDIASOUP_H
#define	MS_MEDIASOUP_H


class MediaSoup {
public:
	static void SetProcess();
	static void Run();
	static void End();

private:
	static void IgnoreSignals();
	static void SetKernelLimits();
	static void SetUserGroup();
	static void ThreadInit();
	static void ThreadDestroy();
	static void ClassInit();
	static void ClassDestroy();
	static void RunWorkerThread(void* data);
};


#endif
