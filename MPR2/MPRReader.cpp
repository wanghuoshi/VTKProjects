#include "MPRReader.h"
#include <QMessageBox>
#include "vtkDICOMDirectory.h"

MPRReader::MPRReader(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    connect(ui.actionFolder, &QAction::triggered, this, &MPRReader::constructMPR);
	
	openFolder();
}

MPRReader::~MPRReader()
{}

void MPRReader::openFolder()
{
	m_path = "D:/Projects/DICOM-Viewer/MPRViewer/MPRViewer/res/SLC/";
	vtkSmartPointer<vtkDICOMDirectory> dicomDir = vtkSmartPointer<vtkDICOMDirectory>::New();
	dicomDir->SetDirectoryName(m_path.c_str());
	dicomDir->Update();
	
	if (dicomDir->GetNumberOfSeries() > 0) {
		m_reader->SetFileNames(dicomDir->GetFileNamesForSeries(0));
		m_reader->SetDataSpacing(1.0, 1.0, 1.0); 
		m_reader->Update();
	}
	else
	{
		QMessageBox::information(this, "错误", "文件夹中没有DICOM图像！");
	}
}

void MPRReader::constructMPR()
{
	ui.widget1->SetInputData(m_reader->GetOutput(), 0);
	ui.widget2->SetInputData(m_reader->GetOutput(), 1);
	ui.widget3->SetInputData(m_reader->GetOutput(), 2);
}