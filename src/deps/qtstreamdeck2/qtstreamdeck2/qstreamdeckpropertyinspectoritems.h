#pragma once

#include <QString>
#include <QMap>
#include <QVariant>

#include "qstreamdeckdeclares.h"
#include "qstreamdeckevent.h"
#include "qstreamdeckaction.h"
#include "qstreamdeckplugin.h"

struct QStreamDeckPropertyInspectorItems {
	friend class QStreamDeckPropertyInspectorBuilder;

protected:
	static inline QString escapeQuotes(const QString &str) {
		QString s = str;
		s.replace("\"", "\\\"");
		return s;
	}

	struct Item {

	public:
		virtual ~Item() = default;

	public:
		QString id;
		QStreamDeckAction *action = nullptr;
		QList<QStreamDeckPropertyInspectorCallback> callbacks;

	public:
		virtual void buildHTML(QString &sink) const = 0;
	};
	using ItemUPtr = std::unique_ptr<Item>;

	template<typename T_>
	struct ItemT : public Item {
		using T = T_;

	public:
		inline T &setID(const QString &set) {
			id = set;
			return ref();
		}

		inline T &addCallback(const QStreamDeckPropertyInspectorCallback &f) {
			this->callbacks += f;
			return ref();
		}

	protected:
		inline T &ref() {
			return static_cast<T &>(*this);
		}

	};
	template<typename T>
	struct ElementItemT : public ItemT<T> {

	public:
		QString label;
		QMap<QString, QString> properties;

	public:
		inline T &setLabel(const QString &set) {
			label = set;
			return this->ref();
		}

		inline T &setProperty(const QString &property, const QString &value) {
			properties.insert(property, value);
			return this->ref();
		}

	protected:
		void buildLabel(QString &sink) const {
			sink += QStringLiteral("<div class=\"sdpi-item-label%2\">%1</div>").arg(label, label.isEmpty() ? " empty" : "");
		}

		void buildPre(QString &sink) const {
			sink += QStringLiteral("<div class=\"sdpi-item\">");
			buildLabel(sink);
		}

		void buildPost(QString &sink) const {
			sink += "</div>";
		}

		void buildProperties(QString &sink) const {
			sink += QStringLiteral(" id=\"%1\" name=\"%1\"").arg(escapeQuotes(this->id));

			for(auto it = properties.begin(), e = properties.end(); it != e; it++)
				sink += QStringLiteral(" %1=\"%2\"").arg(it.key(), escapeQuotes(it.value()));
		}
	};
	template<typename T>
	struct ValueItemT : public ElementItemT<T> {

	public:
		QVariant value;

	public:
		inline T &setValue(const QVariant &set) {
			value = set;
			return this->ref();
		}

		inline T &addValueChangedCallback(const std::function<void(const QVariant &)> &f) {
			return this->addCallback([f](const QStreamDeckEvent &e) {
				if(e.payload["cmd"] == "valueChanged")
					f(e.payload["value"].toVariant());
			});
		}

	public:
		/// Sets the item value to the setting value and adds the value changed callback to update the setting key
		T &linkWithActionSetting(const QString &settingsKey = {}) {
			const QString key = settingsKey.isEmpty() ? this->id : settingsKey;

			return
				setValue(this->action->setting(key).toVariant())
					.addValueChangedCallback(
						[a = this->action, key](const QVariant &v) {
							a->setSetting(key, QJsonValue::fromVariant(v));
						}
					);
		}

		/// Sets the item value to the setting value and adds the value changed callback to update the setting key
		T &linkWithGlobalSetting(const QString &settingsKey = {}) {
			const QString key = settingsKey.isEmpty() ? this->id : settingsKey;
			auto plugin = this->action->plugin();

			return
				setValue(plugin->globalSetting(key).toVariant())
					.addValueChangedCallback(
						[plugin, key](const QVariant &v) {
							plugin->setGlobalSetting(key, QJsonValue::fromVariant(v));
						}
					);
		}

	};

public:
	struct Item_HTML final : public ItemT<Item_HTML> {

	public:
		QString html;

	public:
		inline T &setHTML(const QString &set) {
			html = set;
			return *this;
		}

	public:
		void buildHTML(QString &sink) const override {
			sink += html;
		}
	};
	struct Item_Section final : public ElementItemT<Item_Section> {
		void buildHTML(QString &sink) const override {
			sink += QStringLiteral("<div class=\"sdpi-heading\">%1</div>").arg(label);
		}
	};
	struct Item_Message final : public ElementItemT<Item_Message> {

	public:
		QStringList paragraphs;

	public:
		inline T &addParagraph(const QString &p) {
			paragraphs += p;
			return *this;
		}

		inline T &addParagraphs(const QStringList &p) {
			paragraphs += p;
			return *this;
		}

	public:
		void buildHTML(QString &sink) const override {
			buildPre(sink);
			sink += "<details class=\"sdpi-item-value message\" open>";

			for(const QString &p: paragraphs)
				sink += QStringLiteral("<p>%1</p>").arg(p);

			sink += "</details>";
			buildPost(sink);
		}
	};
	struct Item_LineEdit final : public ValueItemT<Item_LineEdit> {

		inline T &setPlaceholder(const QString &set) {
			setProperty("placeholder", set);
			return this->ref();
		}

		void buildHTML(QString &sink) const override {
			buildPre(sink);
			sink += QStringLiteral("<input type=\"text\" class=\"sdpi-item-value\" oninput=\"notifyValueChanged(this.name, this.value)\"");
			sink += QStringLiteral(" value=\"%1\"").arg(escapeQuotes(this->value.toString()));
			this->buildProperties(sink);
			sink += ">";
			buildPost(sink);
		}
	};
	struct Item_SpinBox final : public ValueItemT<Item_SpinBox> {

		inline T &setPlaceholder(const QString &set) {
			setProperty("placeholder", set);
			return this->ref();
		}

		void buildHTML(QString &sink) const override {
			buildPre(sink);
			sink += QStringLiteral("<input"
			                       " type=\"number\""
			                       " inputtype=\"numeric\""
			                       " pattern=\"[0-9]*\""
			                       " class=\"sdpi-item-value\""
			                       " oninput=\"notifyValueChanged(this.name, parseInt(this.value))\""
			);
			sink += QStringLiteral(" value=\"%1\"").arg(escapeQuotes(this->value.toString()));
			this->buildProperties(sink);
			sink += ">";
			buildPost(sink);
		}
	};

	struct Item_CheckBox final : public ValueItemT<Item_CheckBox> {

	public:
		QString rightSideLabel;

	public:
		/// Extra label on the right side of the checkbox
		inline T &setRightSideLabel(const QString &set) {
			rightSideLabel = set;
			return ref();
		}

	public:
		void buildHTML(QString &sink) const override {
			sink += "<div class=\"sdpi-item\" type=\"checkbox\">";
			buildLabel(sink);
			sink +=
				"<div class=\"sdpi-item-value\">"
				"<input type=\"checkbox\" oninput=\"notifyValueChanged(this.name, this.checked)\"";
			this->buildProperties(sink);
			if(value.toBool())
				sink += "checked";

			sink += QStringLiteral(
				">"
				"<label for=\"%2\"><span></span>%1</label>"
				"</div>"
				"</div>"
			).arg(rightSideLabel, id);
		}
	};


	struct Item_ComboBox final : public ValueItemT<Item_ComboBox> {

	public:
		QStringList items;

	public:
		inline T &setItems(const QStringList &set) {
			items = set;
			return ref();
		}

	public:
		void buildHTML(QString &sink) const override {
			buildPre(sink);
			sink +=
				"<select class=\"sdpi-item-value select\" oninput=\"notifyValueChanged(this.name, this.selectedIndex)\"";
			this->buildProperties(sink);
			sink += ">";

			int i = 0;
			const int val = value.toInt();
			for(const QString &item : items) {
				sink += QStringLiteral("<option value=\"%1\"%3>%2</option>").arg(QString::number(i), item, val == i ? " selected" : "");
				i++;
			}

			sink += "</select>";
			buildPost(sink);
		}
	};
};