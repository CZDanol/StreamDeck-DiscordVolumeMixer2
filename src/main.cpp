#include "dvmplugin.h"

#include <QFile>
#include <QGuiApplication>
#include <QMutex>
#include <QMutexLocker>
#include <QThread>

void messageLogger(QtMsgType type, const QMessageLogContext &, const QString &msg) {
	static QFile lf("log.txt"), clf("clog.txt");
	static QMutex mutex;
	QMutexLocker lock(&mutex);

	if(!lf.isOpen())
		lf.open(QIODevice::WriteOnly);

	if(!clf.isOpen())
	{
		// Keep diagnostics useful without allowing an always-running plugin to
		// grow its log indefinitely.
		if(clf.size() > 2 * 1024 * 1024) {
			QFile::remove("clog.previous.txt");
			QFile::rename("clog.txt", "clog.previous.txt");
		}
		clf.open(QIODevice::Append);
	}

	const char *level = "INFO";
	switch(type) {
		case QtDebugMsg: level = "DEBUG"; break;
		case QtInfoMsg: level = "INFO"; break;
		case QtWarningMsg: level = "WARN"; break;
		case QtCriticalMsg: level = "ERROR"; break;
		case QtFatalMsg: level = "FATAL"; break;
	}
	const QByteArray line = QStringLiteral("%1 [%2] %3\n")
		.arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), QString::fromLatin1(level), msg)
		.toUtf8();

	if(lf.isOpen()) {
		lf.write(line);
		lf.flush();
	}

	if(clf.isOpen()) {
		clf.write(line);
		clf.flush();
	}
}

int main(int argc, char *argv[]) {
	QCoreApplication::setAttribute(Qt::AA_PluginApplication);

	//QThread::sleep(10);

	QGuiApplication app(argc, argv);
	qInstallMessageHandler(&messageLogger);
	qDebug() << QDateTime::currentDateTime().toString() << "Plugin starting";

	DVMPlugin plugin;
	plugin.init("com.thomast.discordmixer", app);

	return QGuiApplication::exec();
}
