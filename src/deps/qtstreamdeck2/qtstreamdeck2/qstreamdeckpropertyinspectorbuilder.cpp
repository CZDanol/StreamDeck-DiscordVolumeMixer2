#include "qstreamdeckpropertyinspectorbuilder.h"

QStreamDeckPropertyInspectorBuilder::QStreamDeckPropertyInspectorBuilder(QStreamDeckAction *action) : action_(action) {

}

QString QStreamDeckPropertyInspectorBuilder::buildHTML() const {
	QString r;
	for(const auto &item: items_)
		item->buildHTML(r);
	return r;
}

QStreamDeckPropertyInspectorCallback QStreamDeckPropertyInspectorBuilder::buildCallback() const {
	QHash<QString, QList<QStreamDeckPropertyInspectorCallback>> ct;
	for(const auto &item: items_) {
		if(item->callbacks.isEmpty())
			continue;

		Q_ASSERT(!item->id.isEmpty());
		ct.insert(item->id, item->callbacks);
	}

	return [ct](const QStreamDeckEvent &e) {
		if(auto fs = ct.value(e.payload["element"].toString()); !fs.isEmpty())
			for(const auto &f: fs)
				f(e);
	};
}
