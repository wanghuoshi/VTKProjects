#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_MPRReader.h"
#include "vtkInclude.h"
#include "dicomInclude.h"

class MPRReader : public QMainWindow
{
    Q_OBJECT

public:
    MPRReader(QWidget *parent = nullptr);
    ~MPRReader();

    void openFolder();

public slots:
    void constructMPR();

private:
    Ui::MPRReaderClass ui;

    std::string m_path;
    vtkSmartPointer<vtkDICOMReader> m_reader = vtkSmartPointer<vtkDICOMReader>::New();
};
