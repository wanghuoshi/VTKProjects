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
	//��ǰ��ͼ��vtkWindow����
	vtkSmartPointer<vtkGenericOpenGLRenderWindow> myWindow;

public:
	//��ǰ��ͼ��vtkRenderer��̨
	vtkSmartPointer<vtkRenderer> myRenderer;
	//��ǰ��ͼ��vtkInteractor������
	vtkSmartPointer<vtkRenderWindowInteractor> myInteractor;
	//��ǰ��ͼʮ�ֹ�����
	vtkSmartPointer<vtkActor> HorizontalLineActor;
	//��ǰ��ͼʮ�ֹ������
	vtkSmartPointer<vtkActor> VerticalLineActor;
	//��ǰ��ͼ�����㷨��
	vtkSmartPointer<vtkImageReslice> myImageReslice;
	vtkSmartPointer<vtkImageMapToColors> myMapToColors;
	vtkSmartPointer<vtkImageActor> myImageActor;
	//��Ӧ����������ͼ�Ķ���ָ��
	MPRWidget* MyWidget1;
	MPRWidget* MyWidget2;

public:
	//���������ݺ����淽������directionΪ0����ʸ״�棬1�����״�棬2������״��
	void SetInputData(vtkImageData* data, int direction);

private:
	//Ui::MPRWidget ui;
};
