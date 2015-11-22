#ifndef MS_MEDIASOUP_H
#define	MS_MEDIASOUP_H

/*
 * MediaSoup top class.
 *
 * Entry point of the application.
 */
class MediaSoup
{
public:
	// Set the process.
	static void SetProcess();
	// Run MediaSoup.
	static void Run();
	// Stop MediaSoup.
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
