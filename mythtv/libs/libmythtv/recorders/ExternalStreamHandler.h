// -*- Mode: c++ -*-

#ifndef _External_Streamhandler_H_
#define _External_Streamhandler_H_

#include <cstdint>
#include <vector>
using namespace std;

#include <QString>
#include <QAtomicInt>
#include <QMutex>
#include <QMap>

#include "streamhandler.h"

class DTVSignalMonitor;
class ExternalChannel;

class ExternIO
{
    enum constants { kMaxErrorCnt = 20 };

  public:
    ExternIO(const QString & app, const QStringList & args);
    ~ExternIO(void);

    bool Ready(int fd, int timeout, const QString & what);
    int Read(QByteArray & buffer, int maxlen, int timeout = 2500);
    QString GetStatus(int timeout = 2500);
    int Write(const QByteArray & buffer);
    bool Run(void);
    bool Error(void) const { return !m_error.isEmpty(); }
    QString ErrorString(void) const { return m_error; }
    void ClearError(void) { m_error.clear(); }

    bool KillIfRunning(const QString & cmd);

  private:
    void Fork(void);

    QFileInfo   m_app;
    QStringList m_args;
    int     m_appin;
    int     m_appout;
    int     m_apperr;
    pid_t   m_pid;
    QString m_error;

    int         m_bufsize;
    char       *m_buffer;

    QString     m_status_buf;
    QTextStream m_status;
    int         m_errcnt;
};

// Note : This class always uses a TS reader.

class ExternalStreamHandler : public StreamHandler
{
    enum constants { MAX_API_VERSION = 2,
                     PACKET_SIZE = 188 * 32768,
                     TOO_FAST_SIZE = 188 * 4196 };

  public:
    static ExternalStreamHandler *Get(const QString &devicename,
                                      int inputid);
    static void Return(ExternalStreamHandler * & ref, int inputid);

  public:
    explicit ExternalStreamHandler(const QString & path, int inputid);
    ~ExternalStreamHandler(void) { CloseApp(); }

    void run(void) override; // MThread
    void PriorityEvent(int fd) override; // DeviceReaderCB

    bool IsAppOpen(void);
    bool IsTSOpen(void);
    bool HasTuner(void) const { return m_hasTuner; }
    bool HasPictureAttributes(void) const { return m_hasPictureAttributes; }

    bool RestartStream(void);

    void LockReplay(void) { m_replay_lock.lock(); }
    void UnlockReplay(bool enable_replay = false)
        { m_replay = enable_replay; m_replay_lock.unlock(); }
    void ReplayStream(void);
    bool StartStreaming(void);
    bool StopStreaming(void);

    bool CheckForError(void);

    void PurgeBuffer(void);

    bool ProcessCommand(const QString & cmd, uint timeout, QString & result,
                        uint retry_cnt = 10, uint wait_cnt = 2);
    bool ProcessVer1(const QString & cmd, uint timeout, QString & result,
                     uint retry_cnt, uint wait_cnt);
    bool ProcessVer2(const QString & cmd, uint timeout, QString & result,
                     uint retry_cnt, uint wait_cnt);

  private:
    int StreamingCount(void) const;
    bool OpenApp(void);
    void CloseApp(void);

    QMutex         m_IO_lock;
    ExternIO      *m_IO;
    QStringList    m_args;
    QString        m_app;
    bool           m_tsopen;
    int            m_io_errcnt;
    bool           m_poll_mode;
    bool           m_notify;

    int            m_apiVersion;
    uint           m_serialNo;
    bool           m_hasTuner;
    bool           m_hasPictureAttributes;

    QByteArray     m_replay_buffer;
    bool           m_replay;

    // for implementing Get & Return
    static QMutex                           m_handlers_lock;
    static QMap<QString, ExternalStreamHandler*> m_handlers;
    static QMap<QString, uint>              m_handlers_refcnt;

    QAtomicInt    m_streaming_cnt;
    QMutex        m_stream_lock;
    QMutex        m_replay_lock;
    QMutex        m_process_lock;
};

#endif // _ExternalSTREAMHANDLER_H_
