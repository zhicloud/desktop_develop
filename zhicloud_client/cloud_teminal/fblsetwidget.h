#ifndef FBLSETWIDGET_H
#define FBLSETWIDGET_H

#include <QWidget>

#include <QPushButton>
#include <QHBoxLayout>
#include <QDebug>
#include <QVBoxLayout>
#include <QVector>
#include <QLabel>
#include <QStackedLayout>
#include <string>

class fblSetWidget : public QWidget
{
	Q_OBJECT

public:
	fblSetWidget(QWidget *parent = NULL);
	~fblSetWidget();
public slots :

	void hideOrShow();
	void setFblIndex(int index);
	void chooseFbl();
public:
	void createView();
	void initView();
private:
	QWidget* ppp;
	QPushButton *fblBtn;
	QPushButton *fblBtn1;
	QPushButton *fblBtn2;
	QPushButton *fblBtn3;
	QPushButton *fblBtn4;
	QPushButton *fblBtn5;
	QVBoxLayout *mainLayout;
	QStackedLayout *hpiclayout;
	QVector<QPushButton *> fblBtn_Vec;
private:
	bool isfblBtnShow;
	int fblIndex;
private:
	
};

#endif // FBLSETWIDGET_H
