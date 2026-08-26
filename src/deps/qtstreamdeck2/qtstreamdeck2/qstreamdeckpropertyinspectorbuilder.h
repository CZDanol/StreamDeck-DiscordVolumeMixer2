#pragma once

#include <QString>
#include <QList>
#include <QMap>

#include "qstreamdeckdeclares.h"
#include "qstreamdeckpropertyinspectoritems.h"

class QStreamDeckPropertyInspectorBuilder {
	using I = QStreamDeckPropertyInspectorItems;

public:
	inline QStreamDeckAction *action() const {
		return action_;
	}

public:
	template<typename T>
	inline T &addItem() {
		auto i = new T;
		i->action = action_;
		items_.emplace_back(i);
		return *i;
	}

	inline auto &addHTML(const QString &html) {
		return addItem<I::Item_HTML>().setHTML(html);
	}

	inline auto &addSection(const QString &label) {
		return addItem<I::Item_Section>().setLabel(label);
	}

	inline auto &addMessage(const QString &text) {
		return addItem<I::Item_Message>().addParagraph(text);
	}

	inline auto &addMessage(const QStringList &paragraphs) {
		return addItem<I::Item_Message>().addParagraphs(paragraphs);
	}

	/// Single line text edit
	inline auto &addLineEdit(const QString &id, const QString &label) {
		return addItem<I::Item_LineEdit>().setID(id).setLabel(label);
	}

	/// Integer input
	inline auto &addSpinBox(const QString &id, const QString &label) {
		return addItem<I::Item_SpinBox>().setID(id).setLabel(label);
	}

	inline auto &addCheckBox(const QString &id, const QString &rightSideLabel) {
		return addItem<I::Item_CheckBox>().setID(id).setRightSideLabel(rightSideLabel);
	}

	inline auto &addComboBox(const QString &id, const QString &label, const QStringList &items) {
		return addItem<I::Item_ComboBox>().setID(id).setLabel(label).setItems(items);
	}

public:
	QStreamDeckPropertyInspectorBuilder(QStreamDeckAction *action);

public:
	QString buildHTML() const;
	QStreamDeckPropertyInspectorCallback buildCallback() const;

private:
	QStreamDeckAction *action_ = nullptr;
	std::vector<I::ItemUPtr> items_;

};
