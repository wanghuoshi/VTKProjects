#pragma once

#include "vtkInclude.h"

class MPRWidget : public QVTKOpenGLNativeWidget
{
	Q_OBJECT

private:
	enum Direction
	{
		X_DIRECTION = 0,
		Y_DIRECTION,
		Z_DIRECTION,
		N_DIRECTION
	};
	vtkSmartPointer<vtkMatrix4x4> AxialResliceMatrix;
	vtkSmartPointer<vtkMatrix4x4> CoronalResliceMatrix;
	vtkSmartPointer<vtkMatrix4x4> SagittalResliceMatrix;

public:
	MPRWidget(QWidget* parent);
	~MPRWidget();

private:
	//当前视图的vtkWindow窗口
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> myWindow;

public:
	//当前视图的vtkRenderer舞台
	vtkSmartPointer<vtkRenderer> myRenderer;
	//当前视图的vtkInteractor交互器
	vtkSmartPointer<vtkRenderWindowInteractor> myInteractor;
	//当前视图十字光标横轴
	vtkSmartPointer<vtkActor> HorizontalLineActor;
	//当前视图十字光标竖轴
	vtkSmartPointer<vtkActor> VerticalLineActor;
	//当前视图切面算法类
	vtkSmartPointer<vtkImageReslice> myImageReslice;
	vtkSmartPointer<vtkImageMapToColors> myMapToColors;
	vtkSmartPointer<vtkImageActor> myImageActor;
	//对应其他两个视图的对象指针
	MPRWidget* MyWidget1;
	MPRWidget* MyWidget2;

public:
	//输入体数据和切面方向，其中direction为0代表矢状面，1代表冠状面，2代表轴状面
	void SetInputData(vtkImageData* data, int direction);

private:
	//Ui::MPRWidget ui;
};
