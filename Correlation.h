// ======================================================================
// IMPROC: Image Processing Software Package
// Copyright (C) 2016 by George Wolberg
//
// Correlation.h - Correlation widget
//
// Written by: George Wolberg, 2016
// ======================================================================

#ifndef CORRELATION_H
#define CORRELATION_H

#include "ImageFilter.h"

class Correlation : public ImageFilter {
	Q_OBJECT

public:
	Correlation(QWidget *parent = 0);	// constructor
	QGroupBox*	controlPanel();			// create control panel
	bool		applyFilter(ImagePtr, bool, ImagePtr);	// apply filter to input
	void		correlation(ImagePtr, ImagePtr, int, int, int&, int&);
	void		initShader();
	void		gpuProgram(int pass);	// use GPU program to apply filter

	protected slots:
	int		load();
	int		match();

private:
	// widgets
	QStackedWidget*		m_stackWidgetImages;
	QPushButton*	m_button1;	// Correlation pushbutton
	QPushButton*	m_button2;	// Correlation pushbutton
	QGroupBox*	m_ctrlGrp;	// groupbox for panel

	// variables
	QString		m_file;
	QString		m_currentDir;
	ImagePtr	m_cimageIn;		// input image (raw)
	int		m_width;	// input image width
	int		m_height;	// input image height
	int		m_xx;
	int		m_yy;
	int		m_width_template;  // template image width
	int		m_high_template;   // template image high
};

#endif	// CORRELATION_H
