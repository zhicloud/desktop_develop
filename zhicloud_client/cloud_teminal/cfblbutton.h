#ifndef CFBLBUTTON_H
#define CFBLBUTTON_H

#include <QPushButton>

class CFblbutton : public QPushButton
{
	Q_OBJECT

public:
	CFblbutton(QWidget *parent = NULL);
	~CFblbutton();
signals:
	void setFblIndex(int );
public slots:
	bool eventFilter(QObject *, QEvent *);
public:
	void setIndex(int index);
	int getIndex();
private:
	int m_index;
};

#endif // CFBLBUTTON_H
