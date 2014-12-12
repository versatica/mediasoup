#ifndef MS_DAEMON_H
#define	MS_DAEMON_H


class Daemon {
public:
	static void Daemonize();
	static void SendOKStatusToAncestor();
	static void SendErrorStatusToAncestor();
	static bool IsDaemonized();
	static void End();

private:
	static bool WritePIDFile();
	static void DeletePIDFile();
	static bool WaitForDaemonizedStatus(char* status);
	static void SendStatusToAncestor(char status);

private:
	static int statusPipe[2];
	static bool isDaemonized;
	static bool hasWrittenPIDFile;
};


#endif
