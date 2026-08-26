#include "qstreamdeckcommand.h"

QString operator +(QStreamDeckCommand::Command c) {
	static const QVector<QString> h = []() {
		QVector<QString> r;
		const int cnt = int(QStreamDeckCommand::Command::_cnt);
		r.resize(cnt);

		const auto me = QMetaEnum::fromType<QStreamDeckCommand::Command>();
		for(int i = 0; i < cnt; i++)
			r[i] = me.valueToKey(i);

		return r;
	}();
	return h.value(c);
}