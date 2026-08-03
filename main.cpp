#include "protocol.hpp"

int main(int argc, char** argv) {
	QApplication app(argc, argv);
	Protocol p("/usr/share/wayland/wayland.xml");

	QTextEdit text_edit;
	text_edit.setReadOnly(true);
	QTextCursor cursor = text_edit.cursorForPosition({});
	QTextBlockFormat block_format;
	block_format.setBottomMargin(5);
	QTextCharFormat protocol_format;
	protocol_format.setFontPointSize(protocol_format.font().pointSize() + 5);
	protocol_format.setFontWeight(QFont::Bold);
	protocol_format.setAnchor(true);
	protocol_format.setAnchorHref(p.name);
	QTextCharFormat interface_format;
	interface_format.setFontPointSize(interface_format.font().pointSize() + 2);
	interface_format.setFontWeight(QFont::Bold);
	QTextCharFormat summary_format;
	summary_format.setFontWeight(QFont::Bold);
	QTextCharFormat content_format;
	QFont monospace("monospace");
	QTextCharFormat monospace_format;
	monospace_format.setFont(monospace);

	auto insertDescription = [&] (Description const& description) {
		if (!description.summary.isEmpty()) {
			cursor.insertText(description.summary, summary_format);
			cursor.insertBlock();
		}
		if (!description.content.isEmpty()) {
			cursor.insertMarkdown(description.content);
			cursor.insertBlock();
		}
	};
	cursor.setBlockFormat(block_format);
	cursor.insertText(p.name, protocol_format);
	cursor.insertBlock();
	insertDescription(p.description);
	for (Interface &interface: p.interfaces) {
		cursor.insertText(interface.name, interface_format);
		cursor.insertBlock();
		insertDescription(interface.description);
		for (Request &request: interface.requests) {
			cursor.insertText(request.signature(), monospace_format);
			cursor.insertBlock();
			insertDescription(request.description);
			cursor.insertBlock();
		}
		for (Event &event: interface.events) {
			cursor.insertText(event.signature(), monospace_format);
			cursor.insertBlock();
			insertDescription(event.description);
			cursor.insertBlock();
		}
		for (Enum &e: interface.enums) {
			cursor.insertText(e.name, monospace_format);
			cursor.insertBlock();
			QTextTable* table = cursor.insertTable(e.entries.size(), 3);
			for (size_t i = 0; i < e.entries.size(); i++) {
				Entry const& entry = e.entries[i];
				cursor.insertText(entry.name);
				cursor.movePosition(QTextCursor::NextCell);
				cursor.insertText(entry.value);
				cursor.movePosition(QTextCursor::NextCell);
				cursor.insertText(entry.summary);
				cursor.movePosition(QTextCursor::NextCell);
			}
			cursor.movePosition(QTextCursor::Down);
			insertDescription(e.description);
			cursor.insertBlock();
		}
	}
	cursor.insertText("Copyright", summary_format);
	cursor.insertMarkdown(p.copyright);
	text_edit.scrollToAnchor(p.name);
	text_edit.setWindowTitle(p.name);
	text_edit.show();
	return app.exec();
}

// Local Variables:
// c-file-style: "awk"
// compile-command: "ninja -C build && build/QtDocview"
// End:
