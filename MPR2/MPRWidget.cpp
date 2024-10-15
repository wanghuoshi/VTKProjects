#include "MPRWidget.h"

//Öá×´Ãæ
double Axial[16] = {
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 40,
		0, 0, 0, 1 };
//¹Ú×´Ãæ
double Coronal[16] = {
		1, 0, 0, 0,
		0, 0, -1, 60,
		0, 1, 0, 0,
		0, 0, 0, 1 };
//Ê¸×´Ãæ
double Sagittal[16] = {
		0, 0, 1, 60,
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 0, 1 };

MPRWidget::MPRWidget(QWidget* parent)
{
	myWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New(); 
	myWindow->SetSize(640, 480);
	this->setRenderWindow(myWindow);

	myRenderer = vtkSmartPointer<vtkRenderer>::New();
	//myRenderer->SetRenderWindow(myWindow);
	myWindow->AddRenderer(myRenderer);

	//myInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	//myInteractor->SetRenderWindow(myWindow);

	myImageReslice = vtkSmartPointer<vtkImageReslice>::New();

	myImageActor = vtkSmartPointer<vtkImageActor>::New();

	AxialResliceMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
	CoronalResliceMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
	SagittalResliceMatrix = vtkSmartPointer<vtkMatrix4x4>::New();

	AxialResliceMatrix->DeepCopy(Axial);
	CoronalResliceMatrix->DeepCopy(Coronal);
	SagittalResliceMatrix->DeepCopy(Sagittal);
}

MPRWidget::~MPRWidget()
{
}

void MPRWidget::SetInputData(vtkImageData* data, int direction)
{
	printf("*-");
	int dims[3];
	data->GetDimensions(dims); printf("%d %d %d", dims[0], dims[1], dims[2]);
	myImageReslice->SetInputData(data);
	switch(direction)
	{
		case X_DIRECTION:
			//SagittalResliceMatrix->SetElement(4, 3, dims[2]/2);
			myImageReslice->SetResliceAxes(SagittalResliceMatrix); printf("0");
			break;
		case Y_DIRECTION:
			//CoronalResliceMatrix->SetElement(3, 2, dims[1]/2);
			myImageReslice->SetResliceAxes(CoronalResliceMatrix); printf("1");
			break;
		case Z_DIRECTION:
			//AxialResliceMatrix->SetElement(3, 2, dims[2]/2);
			myImageReslice->SetResliceAxes(AxialResliceMatrix); printf("2");
			break;
	}
	
	myImageReslice->SetOutputDimensionality(2);
	myImageReslice->SetInterpolationModeToLinear();

	vtkSmartPointer<vtkLookupTable> myLookupTable = vtkSmartPointer<vtkLookupTable>::New();
	myLookupTable->SetTableRange(0, 4800);
	myLookupTable->SetValueRange(0, 1.0);
	myLookupTable->SetSaturationRange(0.0, 1.0);
	myLookupTable->SetRampToLinear();
	myLookupTable->Build();

	vtkSmartPointer<vtkImageMapToColors> myMapToColors = vtkSmartPointer<vtkImageMapToColors>::New();
	myMapToColors->SetLookupTable(myLookupTable);
	myMapToColors->SetInputConnection(myImageReslice->GetOutputPort());
	myMapToColors->Update();

	myImageActor->SetInputData(myImageReslice->GetOutput());
	myRenderer->AddActor(myImageActor);
	myRenderer->SetBackground(1, 1, 1);
	myWindow->FullScreenOff();
	myRenderer->Render();
	myRenderer->ResetCamera();
	//myInteractor->Initialize();
	//myInteractor->Start();
	
}
