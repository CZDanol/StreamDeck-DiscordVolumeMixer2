#include "dvmplugin.h"

#include <QFile>
#include <QGuiApplication>
#include <QThread>
#include <QSslSocket>

void messageLogger(QtMsgType t, const QMessageLogContext &, const QString &msg) {
	static QFile lf("log.txt"), clf("clog.txt");

	// Only create log files if they already exist
	if(!lf.isOpen() && lf.exists())
		lf.open(QIODevice::WriteOnly);

	if(!clf.isOpen() && clf.exists())
		clf.open(QIODevice::Append);

	if(lf.isOpen()) {
		lf.write(msg.toUtf8());
		lf.write("\n");
		lf.flush();
	}

	if(clf.isOpen()) {
		clf.write(msg.toUtf8());
		clf.write("\n");
		clf.flush();
	}
}

int main(int argc, char *argv[]) {
	QCoreApplication::setAttribute(Qt::AA_PluginApplication);

	//QThread::sleep(10);

	QGuiApplication app(argc, argv);
	QCoreApplication::addLibraryPath(QCoreApplication::applicationDirPath());
	qInstallMessageHandler(&messageLogger);
	qDebug() << QDateTime::currentDateTime().toString() << "Plugin starting";
	qDebug() << "Qt library paths" << QCoreApplication::libraryPaths();
	qDebug() << "Available TLS backends" << QSslSocket::availableBackends();
	if(QSslSocket::availableBackends().contains("schannel"))
		QSslSocket::setActiveBackend("schannel");
	qDebug() << "Active TLS backend" << QSslSocket::activeBackend();

	DVMPlugin plugin;
	plugin.init("cz.danol.discordmixer", app);

	return QGuiApplication::exec();
}
