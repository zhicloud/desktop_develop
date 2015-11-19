#ifndef CNETABOUTSPILTER_H
#define CNETABOUTSPILTER_H

#include <QWidget>
#include <QLabel>
class CNetAboutSpilter : public QWidget
{
	Q_OBJECT

public:
	CNetAboutSpilter(QWidget *parent = NULL);
	~CNetAboutSpilter();
	void moveSlider(int index);
private:
	QLabel*		slider;
};

#endif // CNETABOUTSPILTER_H
