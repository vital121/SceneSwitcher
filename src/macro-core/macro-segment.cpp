#include "macro-segment.hpp"
#include "section.hpp"
#include "utility.hpp"
#include "mouse-wheel-guard.hpp"

#include <obs.hpp>
#include <QEvent>
#include <QMouseEvent>
#include <QLabel>
#include <QScrollBar>
#include <QApplication>

namespace advss {

MacroSegment::MacroSegment(Macro *m, bool supportsVariableValue)
	: _macro(m), _supportsVariableValue(supportsVariableValue)
{
}

bool MacroSegment::Save(obs_data_t *obj) const
{
	obs_data_set_bool(obj, "collapsed", static_cast<int>(_collapsed));
	return true;
}

bool MacroSegment::Load(obs_data_t *obj)
{
	_collapsed = obs_data_get_bool(obj, "collapsed");
	return true;
}

bool MacroSegment::PostLoad()
{
	return true;
}

std::string MacroSegment::GetShortDesc() const
{
	return "";
}

void MacroSegment::SetHighlight()
{
	_highlight = true;
}

bool MacroSegment::Highlight()
{
	if (_highlight) {
		_highlight = false;
		return true;
	}
	return false;
}

std::string MacroSegment::GetVariableValue() const
{
	if (_supportsVariableValue) {
		return _variableValue;
	}
	return "";
}

void MacroSegment::SetVariableValue(const std::string &value)
{
	if (_variableRefs > 0) {
		_variableValue = value;
	}
}

void MacroSegment::IncrementVariableRef()
{
	if (!_supportsVariableValue) {
		return;
	}
	_variableRefs++;
}

void MacroSegment::DecrementVariableRef()
{
	if (!_supportsVariableValue || _variableRefs == 0) {
		return;
	}
	_variableRefs--;
}

MacroSegmentEdit::MacroSegmentEdit(bool highlight, QWidget *parent)
	: QWidget(parent),
	  _section(new Section(300)),
	  _headerInfo(new QLabel()),
	  _frame(new QWidget),
	  _contentLayout(new QVBoxLayout),
	  _noBorderframe(new QFrame),
	  _borderFrame(new QFrame),
	  _dropLineAbove(new QFrame),
	  _dropLineBelow(new QFrame),
	  _showHighlight(highlight)
{
	_dropLineAbove->setLineWidth(3);
	_dropLineAbove->setFixedHeight(11);
	_dropLineBelow->setLineWidth(3);
	_dropLineBelow->setFixedHeight(11);

	_borderFrame->setObjectName("border");
	_borderFrame->setStyleSheet("#border {"
				    "border-color: rgba(0, 0, 0, 255);"
				    "border-width: 2px;"
				    "border-style: dashed;"
				    "border-radius: 4px;"
				    "background-color: rgba(0,0,0,100);"
				    "}");
	_noBorderframe->setObjectName("noBorder");
	_noBorderframe->setStyleSheet("#noBorder {"
				      "border-color: rgba(0, 0, 0, 0);"
				      "border-width: 2px;"
				      "border-style: dashed;"
				      "border-radius: 4px;"
				      "background-color: rgba(0,0,0,50);"
				      "}");
	_frame->setObjectName("frameWrapper");
	_frame->setStyleSheet("#frameWrapper {"
			      "border-width: 2px;"
			      "border-radius: 4px;"
			      "background-color: rgba(0,0,0,0);"
			      "}");

	// Set background of these widget types to be transparent to avoid
	// blocking highlight frame background
	setStyleSheet("QCheckBox { background-color: rgba(0,0,0,0); }"
		      "QLabel { background-color: rgba(0,0,0,0); }"
		      "QSlider { background-color: rgba(0,0,0,0); }");

	// Keep the size of macro segments consistent, even if there is room in
	// the edit areas
	setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	QWidget::connect(_section, &Section::Collapsed, this,
			 &MacroSegmentEdit::Collapsed);
	// Macro signals
	QWidget::connect(parent, SIGNAL(MacroAdded(const QString &)), this,
			 SIGNAL(MacroAdded(const QString &)));
	QWidget::connect(parent, SIGNAL(MacroRemoved(const QString &)), this,
			 SIGNAL(MacroRemoved(const QString &)));
	QWidget::connect(
		parent, SIGNAL(MacroRenamed(const QString &, const QString &)),
		this, SIGNAL(MacroRenamed(const QString &, const QString &)));
	// Scene group signals
	QWidget::connect(parent, SIGNAL(SceneGroupAdded(const QString &)), this,
			 SIGNAL(SceneGroupAdded(const QString &)));
	QWidget::connect(parent, SIGNAL(SceneGroupRemoved(const QString &)),
			 this, SIGNAL(SceneGroupRemoved(const QString &)));
	QWidget::connect(
		parent,
		SIGNAL(SceneGroupRenamed(const QString &, const QString)), this,
		SIGNAL(SceneGroupRenamed(const QString &, const QString)));

	auto frameLayout = new QGridLayout;
	frameLayout->setContentsMargins(0, 0, 0, 0);
	frameLayout->addLayout(_contentLayout, 0, 0);
	frameLayout->addWidget(_noBorderframe, 0, 0);
	frameLayout->addWidget(_borderFrame, 0, 0);
	auto layout = new QVBoxLayout;
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(_dropLineAbove);
	layout->addLayout(frameLayout);
	layout->addWidget(_dropLineBelow);
	_frame->setLayout(layout);

	SetSelected(false);
	ShowDropLine(DropLineState::NONE);

	// Enable dragging while clicking on the header text
	_headerInfo->installEventFilter(this);

	_timer.setInterval(1500);
	connect(&_timer, SIGNAL(timeout()), this, SLOT(Highlight()));
	_timer.start();
}

bool MacroSegmentEdit::eventFilter(QObject *obj, QEvent *ev)
{
	if (obj == _headerInfo && ev->type() == QEvent::MouseMove) {
		if (!parentWidget()) {
			return QWidget::eventFilter(obj, ev);
		}
		// Generate a mouse move event for the MacroSegmentList
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(ev);
		QMouseEvent *newEvent = new QMouseEvent(
			mouseEvent->type(),
			_headerInfo->mapTo(this, mouseEvent->pos()),
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
			mouseEvent->globalPos(),
#else
			mouseEvent->globalPosition(),
#endif
			mouseEvent->button(), mouseEvent->buttons(),
			mouseEvent->modifiers());
		QApplication::sendEvent(parentWidget(), newEvent);
	}
	return QWidget::eventFilter(obj, ev);
}

void MacroSegmentEdit::HeaderInfoChanged(const QString &text)
{
	_headerInfo->setVisible(!text.isEmpty());
	_headerInfo->setText(text);
}

void MacroSegmentEdit::Collapsed(bool collapsed)
{
	if (Data()) {
		Data()->SetCollapsed(collapsed);
	}
}

void MacroSegmentEdit::Highlight()
{
	if (!Data()) {
		return;
	}

	if (_showHighlight && Data()->Highlight()) {
		PulseWidget(this, Qt::green, QColor(0, 0, 0, 0), true);
	}
}

void MacroSegmentEdit::EnableHighlight(bool value)
{
	_showHighlight = value;
}

void MacroSegmentEdit::SetFocusPolicyOfWidgets()
{
	QList<QWidget *> widgets = this->findChildren<QWidget *>();
	for (auto w : widgets) {
		PreventMouseWheelAdjustWithoutFocus(w);
	}
}

void MacroSegmentEdit::SetCollapsed(bool collapsed)
{
	_section->SetCollapsed(collapsed);
}

void MacroSegmentEdit::SetSelected(bool selected)
{
	_borderFrame->setVisible(selected);
	_noBorderframe->setVisible(!selected);
}

void MacroSegmentEdit::ShowDropLine(DropLineState state)
{
	switch (state) {
	case MacroSegmentEdit::DropLineState::NONE:
		_dropLineAbove->setFrameShadow(QFrame::Plain);
		_dropLineAbove->setFrameShape(QFrame::NoFrame);
		_dropLineBelow->hide();
		break;
	case MacroSegmentEdit::DropLineState::ABOVE:
		_dropLineAbove->setFrameShadow(QFrame::Sunken);
		_dropLineAbove->setFrameShape(QFrame::Panel);
		_dropLineBelow->hide();
		break;
	case MacroSegmentEdit::DropLineState::BELOW:
		_dropLineAbove->setFrameShadow(QFrame::Plain);
		_dropLineAbove->setFrameShape(QFrame::NoFrame);
		_dropLineBelow->setFrameShadow(QFrame::Sunken);
		_dropLineBelow->setFrameShape(QFrame::Panel);
		_dropLineBelow->show();
		break;
	default:
		break;
	}
}

} // namespace advss
