// ======================================================================
// IMPROC: Image Processing Software Package
// Copyright (C) 2016 by George Wolberg
//
// Correlation.cpp - Correlation widget.
//
// Written by: George Wolberg, 2016
// ======================================================================

#include "MainWindow.h"
#include "Correlation.h"
#include "hw2/HW_correlation.cpp"

extern MainWindow *g_mainWindowP;
enum { SIZEW_T, SIZEH_T, STEPX, STEPY, STEPX_T, STEPY_T, SUM_KERNEL, SAMPLER, SAMPLER_KERNEL };
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Correlation::Correlation:
//
// Constructor.
//
Correlation::Correlation(QWidget *parent) : ImageFilter(parent)
{
	m_cimageIn = NULL;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Correlation::controlPanel:
//
// Create group box for control panel.
//
QGroupBox*
Correlation::controlPanel()
{
	// init group box
	m_ctrlGrp = new QGroupBox("Correlation");

	// layout for assembling filter widget
	QVBoxLayout *vbox = new QVBoxLayout;

	// create file pushbutton
	m_button1 = new QPushButton("File");
	// create match pushbutton
	m_button2 = new QPushButton("Match");

	m_stackWidgetImages = new QStackedWidget;
	m_stackWidgetImages->addWidget(new QLabel);

	QLabel *label;
	label = (QLabel *)m_stackWidgetImages->widget(0); label->setAlignment(Qt::AlignCenter);

	vbox->addWidget(m_button1);
	vbox->addWidget(m_button2);
	vbox->addWidget(m_stackWidgetImages);
	m_ctrlGrp->setLayout(vbox);

	// init signal/slot connections
	connect(m_button1, SIGNAL(clicked()), this, SLOT(load()));
	connect(m_button2, SIGNAL(clicked()), this, SLOT(match()));

	return(m_ctrlGrp);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Correlation::applyFilter:
//
// Run filter on the image, transforming I1 to I2.
// Overrides ImageFilter::applyFilter().
// Return 1 for success, 0 for failure.
//
bool
Correlation::applyFilter(ImagePtr I1, bool gpuFlag, ImagePtr I2)
{
	// error checking
	if (I1.isNull())		return 0;
	if (m_cimageIn.isNull())	return 0;
	m_width = I1->width();
	m_height = I1->height();
	// correlation image
	if (!(gpuFlag && m_shaderFlag))
		correlation(I1, I2, 0, 0, m_xx, m_yy);
	else    g_mainWindowP->glw()->applyFilterGPU(m_nPasses);

	return 1;
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Correlation::correlation:
//
// Correlation image I1 with convolution filter in kernel.
// Output is in I2.
//
void
Correlation::correlation(ImagePtr I1, ImagePtr I2, int mtd, int multires, int &xx, int &yy)
{
	if (m_cimageIn==NULL){
		if (I1 != I2){
			IP_copyImage(I1, I2);
		}
		return;
	}
	float corr = HW_correlation(I1, m_cimageIn, mtd, multires, xx, yy);
	IP_copyImageHeader(I1, I2);
	IP_copyImage(I1, I2);
	int w = I2->width();
	int h = I2->height();
	m_width = m_cimageIn->width();
	m_height = m_cimageIn->height();
	int type;
	ChannelPtr<uchar> p2, p3, endd;
	for (int ch = 0; IP_getChannel(I2, ch, p2, type); ch++) {
		IP_getChannel(m_cimageIn, ch, p3, type);
		for (int i = 0; i < w; i++){
			for (int j = 0; j < h; j++){
				if (i < yy){
					*p2++ = *p2 / 2;
				}
				else if (i >= yy && i <= (yy + m_height - 1)){
					if (j < xx){
						*p2++ = *p2 / 2;
					}
					else if (j > (xx + m_width)){
						*p2++ = *p2 / 2;
					}
					else {
						*p2++;
					}
				}
				else{
					*p2++ = *p2 / 2;
				}
			}
		}
	}
}



// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Correlation::load:
//
// Slot to load filter kernel from file.
//
int
Correlation::load()
{
	QFileDialog dialog(this);

	// open the last known working directory
	if (!m_currentDir.isEmpty())
		dialog.setDirectory(m_currentDir);

	// display existing files and directories
	dialog.setFileMode(QFileDialog::ExistingFile);

	// invoke native file browser to select file
	m_file = dialog.getOpenFileName(this,
		"Open File", m_currentDir,
		"Images (*.jpg *.png *.ppm *.pgm *.bmp);;All files (*)");

	// verify that file selection was made
	if (m_file.isNull()) return 0;

	// save current directory
	QFileInfo f(m_file);
	m_currentDir = f.absolutePath();

	
	// update button with filename (without path)
	m_button1->setText(f.fileName());
	m_button1->update();

	// read input image
	m_cimageIn = IP_readImage(qPrintable(m_file));
	IP_castImage(m_cimageIn, BW_IMAGE, m_cimageIn);
	m_width_template = m_cimageIn->width();
	m_high_template = m_cimageIn->height(); 

	// init view window dimensions
	int ww = m_stackWidgetImages->width();
	int hh = m_stackWidgetImages->height();

	// convert from ImagePtr to QImage to Pixmap
	QImage q;
	IP_IPtoQImage(m_cimageIn, q);
	g_mainWindowP->glw()->setTemplateTexture(q);
	// convert from QImage to Pixmap; rescale if image is larger than view window
	QPixmap p;
	if (MIN(m_width_template, m_high_template) > MIN(ww, hh))
		p = QPixmap::fromImage(q.scaled(QSize(ww, hh), Qt::KeepAspectRatio));
	else	p = QPixmap::fromImage(q);

	// assign pixmap to label widget for display
	QLabel *widget = (QLabel *)m_stackWidgetImages->currentWidget();
	widget->setPixmap(p);

	// apply filter to source image and display result
	g_mainWindowP->preview();

	return 1;
}

int 
Correlation::match(){
	g_mainWindowP->glw()->setcorrDstImage(m_width_template, m_high_template);
	g_mainWindowP->displayOut();
	return 1;
}
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Correlation::initShader:
//
// init shader program and parameters.
//
void
Correlation::initShader()
{
	m_nPasses = 1;

	// initialize GL function resolution for current context
	initializeGLFunctions();

	UniformMap uniforms;

	// init uniform hash table based on uniform variable names and location IDs
	uniforms["u_Wsize_T"] = SIZEW_T;
	uniforms["u_Hsize_T"] = SIZEH_T;

	uniforms["u_StepX"] = STEPX;
	uniforms["u_StepY"] = STEPY;
	uniforms["u_StepX_T"] = STEPX_T;
	uniforms["u_StepY_T"] = STEPY_T;
	uniforms["u_Sampler"] = SAMPLER;
	uniforms["u_Sampler_kernel"] = SAMPLER_KERNEL;
	uniforms["u_sum_kernel"] = SUM_KERNEL;

	g_mainWindowP->glw()->initShader(m_program[PASS1],
		QString(":/hw2/vshader_correlation.glsl"),
		QString(":/hw2/fshader_correlation.glsl"),
		uniforms,
		m_uniform[PASS1]);
	//uniforms.clear();

	m_shaderFlag = true;
}


// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Correlation::gpuProgram:
//
// Active gpu program
//
void
Correlation::gpuProgram(int pass)
{
	int type;
	ChannelPtr<uchar> p;
	IP_getChannel(m_cimageIn, 0, p, type);

	float w = m_cimageIn->width();
	float h = m_cimageIn->height();
	int total = w*h;

	float sum_kernel = 0.0;
	float temp = 0.0;
	for (int i = 0; i<total; i++){
		temp = (float)*p / 255;
		sum_kernel += temp * temp;
		p++;
	}

	sum_kernel = sqrt(sum_kernel);

	glUseProgram(m_program[pass].programId());
	glUniform1f(m_uniform[pass][SUM_KERNEL], sum_kernel);
	glUniform1i(m_uniform[pass][SIZEW_T], w);
	glUniform1i(m_uniform[pass][SIZEH_T], h);
	glUniform1f(m_uniform[pass][STEPX], (GLfloat) 1.0f / m_width);
	glUniform1f(m_uniform[pass][STEPY], (GLfloat) 1.0f / m_height);
	glUniform1f(m_uniform[pass][STEPX_T], (GLfloat) 1.0f / w);
	glUniform1f(m_uniform[pass][STEPY_T], (GLfloat) 1.0f / h);

	glUniform1i(m_uniform[pass][SAMPLER], 0);
	glUniform1i(m_uniform[pass][SAMPLER_KERNEL], 2);
}
