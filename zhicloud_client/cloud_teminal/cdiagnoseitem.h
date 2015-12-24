#ifndef CDIAGNOSEITEM_H
#define CDIAGNOSEITEM_H
#include <QHBoxLayout>
#include <QWidget>
#include <QLabel>
class CDiagnoseItem : public QWidget
{
	Q_OBJECT

public:
	CDiagnoseItem(QWidget *parent = NULL);
	~CDiagnoseItem();
	void setText(QString txt);
	void setStatus(bool torf);
private:
	QLabel* text;
	QLabel* icon;
	QLabel* status;
};

#endif // CDIAGNOSEITEM_H
