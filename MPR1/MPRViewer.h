#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MPRViewer.h"
#include "vtkInclude.h"
#include "dicomInclude.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MPRViewerClass; };
QT_END_NAMESPACE

class MPRViewer : public QMainWindow
{
    Q_OBJECT

public:
    MPRViewer(QWidget *parent = nullptr);
    ~MPRViewer();

public slots:
	//qaction�Ĳۺ���
	void openFolder();
	void showCursorReslize(bool flag); //��ʾ���
	void init();

	//������ƽ���ؽ�
	void constructMPR();
	void constructMPR2();

	//���������ؽ�
	void constructCPR();

private:
	//void initBoxWidget();
	//void initBoxWidgetCallback();

    Ui::MPRViewerClass *ui;

	std::string m_path = {};

	vtkSmartPointer<vtkDICOMReader> m_reader = vtkSmartPointer<vtkDICOMReader>::New();
	vtkSmartPointer<vtkTextActor> mViewImage2D[3];

	vtkSmartPointer<vtkResliceImageViewer> riw[3];
	vtkSmartPointer<vtkImagePlaneWidget> m_planeWidget[3];
	vtkSmartPointer<vtkTextProperty> sliceTextProp[3];
	vtkSmartPointer<vtkTextMapper> sliceTextMapper[3];
	vtkSmartPointer<vtkActor2D> sliceTextActor[3];
	//vtkSmartPointer<vtkResliceImageViewerMeasurements> ResliceMeasurements;
};
