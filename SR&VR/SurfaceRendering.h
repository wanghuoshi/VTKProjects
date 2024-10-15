#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_SurfaceRendering.h"
#include "ui_ResliceView.h"
#include "vtkInclude.h"
#include "dicomInclude.h"

#include <vtkAutoInit.h>
VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingVolumeOpenGL2)


class SurfaceRendering : public QMainWindow
{
    Q_OBJECT

public:
    SurfaceRendering(QWidget *parent = nullptr);
    ~SurfaceRendering();

public slots:
    void mcValueChanged(int value);
    void mc();
    void dc();
    void rayCasting();
    void textureMapping();
    void MIP();
    void cpr();

private:
    Ui::SurfaceRenderingClass ui;

    vtkSmartPointer<vtkDICOMReader> m_reader = vtkSmartPointer<vtkDICOMReader>::New();
    std::string m_path = {};
    vtkSmartPointer< vtkMarchingCubes > boneExtractor;
    vtkSmartPointer<vtkMatrix4x4> AxialResliceMatrix;
};
