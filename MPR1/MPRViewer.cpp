#include <QMessageBox>
#include "MPRViewer.h"
#include <sstream>

//切片状态信息
class StatusMessage {
public:
    static std::string Format(int slice, int maxSlice) {
        std::stringstream tmp;
        tmp << " Slice Number " << slice + 1 << "/" << maxSlice + 1;
        return tmp.str();
    }
};

//------------------------------------------------------------------------------
class vtkResliceCursorCallback : public vtkCommand
{
public:
    static vtkResliceCursorCallback* New() { return new vtkResliceCursorCallback; }

    void Execute(vtkObject* caller, unsigned long ev, void* callData) override
    {

        if (ev == vtkResliceCursorWidget::WindowLevelEvent) //|| ev == vtkCommand::WindowLevelEvent
            //ev == vtkResliceCursorWidget::ResliceThicknessChangedEvent)
        {
            // Render everything
            for (int i = 0; i < 3; i++)
            {
                this->RCW[i]->Render();
            }
            this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
            return;
        }

        //vtkImagePlaneWidget* ipw = dynamic_cast<vtkImagePlaneWidget*>(caller);
        //if (ipw)
        //{
        //    double* wl = static_cast<double*>(callData);

        //    if (ipw == this->IPW[0])
        //    {
        //        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
        //        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
        //    }
        //    else if (ipw == this->IPW[1])
        //    {
        //        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        //        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
        //    }
        //    else if (ipw == this->IPW[2])
        //    {
        //        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        //        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
        //    }
        //    printf("****************");
        //}

        //同步ImageViewer与3d空间
        vtkResliceCursorWidget* rcw = dynamic_cast<vtkResliceCursorWidget*>(caller);
        if (rcw)
        {
            vtkResliceCursorLineRepresentation* rep = dynamic_cast<vtkResliceCursorLineRepresentation*>(rcw->GetRepresentation());

            // Although the return value is not used, we keep the get calls
            // in case they had side-effects
            rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();

            for (int i = 0; i < 3; i++)
            {
                vtkPlaneSource* ps = static_cast<vtkPlaneSource*>(this->IPW[i]->GetPolyDataAlgorithm());
                ps->SetOrigin(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetOrigin());
                ps->SetPoint1(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint1());
                ps->SetPoint2(this->RCW[i]->GetResliceCursorRepresentation()->GetPlaneSource()->GetPoint2());

                // If the reslice plane has modified, update it on the 3D widget
                this->IPW[i]->UpdatePlacement();
            }
            printf("...............");
        }

        vtkResliceImageViewer* riw = dynamic_cast<vtkResliceImageViewer*>(caller);
        if (riw)
        {
            printf("...............");
        }

        // Render everything
        for (int i = 0; i < 3; i++)
        {
            this->RCW[i]->Render();
        }
        this->IPW[0]->GetInteractor()->GetRenderWindow()->Render();
    }

    vtkResliceCursorCallback() {}
    vtkImagePlaneWidget* IPW[3];
    vtkResliceCursorWidget* RCW[3];
    int Slice;
    int MinSlice;
    int MaxSlice;
};

MPRViewer::MPRViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MPRViewerClass())
{
    ui->setupUi(this);

    init();
}

MPRViewer::~MPRViewer()
{
    delete ui;
}

void MPRViewer::init()
{
    connect(ui->actionFolder, &QAction::triggered, this, &MPRViewer::openFolder);
    connect(ui->actionReslice, &QAction::triggered, this, &MPRViewer::showCursorReslize);
    connect(ui->cprBtn, &QPushButton::clicked, this, &MPRViewer::constructCPR);
}

void MPRViewer::showCursorReslize(bool mode)
{
    for (int i = 0; i < 3; i++)
    {
        vtkResliceCursorLineRepresentation* rep = dynamic_cast<vtkResliceCursorLineRepresentation*>(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        for (int i = 0; i < 3; i++) rep->GetResliceCursorActor()->GetCenterlineProperty(i)->SetOpacity(mode);
        if (mode) riw[i]->GetResliceCursorWidget()->On();
        else riw[i]->GetResliceCursorWidget()->Off();
        riw[i]->GetRenderer()->ResetCamera();
        riw[i]->Render();
    }
    printf("%d", mode);
}

void MPRViewer::openFolder()
{
	m_path = "D:/Projects/DICOM-Viewer/MPRViewer/MPRViewer/res/SLC";
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

	constructMPR();
}

void MPRViewer::constructMPR()
{
    for (int i = 0; i < 3; i++)
    {
        riw[i] = vtkSmartPointer<vtkResliceImageViewer>::New();
        vtkNew<vtkGenericOpenGLRenderWindow> renderWin;
        riw[i]->SetRenderWindow(renderWin);
    }

    ui->openGLWidget_1->setRenderWindow(riw[0]->GetRenderWindow());
    riw[0]->SetupInteractor(ui->openGLWidget_1->renderWindow()->GetInteractor());
    ui->openGLWidget_2->setRenderWindow(riw[1]->GetRenderWindow());
    riw[1]->SetupInteractor(ui->openGLWidget_2->renderWindow()->GetInteractor());
    ui->openGLWidget_3->setRenderWindow(riw[2]->GetRenderWindow());
    riw[2]->SetupInteractor(ui->openGLWidget_3->renderWindow()->GetInteractor());

    for (int i = 0; i < 3; i++)
    {
        vtkResliceCursorLineRepresentation* rep = vtkResliceCursorLineRepresentation::SafeDownCast(
            riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation());

        rep->GetResliceCursorActor()->GetCenterlineProperty(0)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->GetCenterlineProperty(1)->SetRepresentationToWireframe();
        rep->GetResliceCursorActor()->GetCenterlineProperty(2)->SetRepresentationToWireframe();

        rep->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(i); //设置切哪个平面
        
        riw[i]->SetResliceCursor(riw[0]->GetResliceCursor());
        riw[i]->SetInputData(m_reader->GetOutput());
        riw[i]->SetSliceOrientation(i);
        riw[i]->SetResliceModeToAxisAligned(); //切割模式为垂直切
    }

    vtkSmartPointer<vtkCellPicker> picker = vtkSmartPointer<vtkCellPicker>::New();
    picker->SetTolerance(0.005);

    vtkSmartPointer<vtkProperty> ipwProp = vtkSmartPointer<vtkProperty>::New();

    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWin = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    ui->openGLWidget_4->setRenderWindow(renderWin);
    ui->openGLWidget_4->renderWindow()->AddRenderer(ren);

    int imageDims[3];
    m_reader->GetOutput()->GetDimensions(imageDims);

    for (int i = 0; i < 3; i++)
    {
        //当前切片数文本
        sliceTextProp[i] = vtkSmartPointer<vtkTextProperty>::New();
        sliceTextProp[i]->SetFontFamilyToCourier();
        sliceTextProp[i]->SetFontSize(15);
        sliceTextProp[i]->SetVerticalJustificationToBottom();
        sliceTextProp[i]->SetJustificationToLeft();

        sliceTextMapper[i] = vtkSmartPointer<vtkTextMapper>::New();
        std::string msg1 = StatusMessage::Format(imageDims[2] / 2, riw[i]->GetSliceMax());
        sliceTextMapper[i]->SetInput(msg1.c_str());
        sliceTextMapper[i]->SetTextProperty(sliceTextProp[i]);

        sliceTextActor[i] = vtkSmartPointer<vtkActor2D>::New();
        sliceTextActor[i]->SetMapper(sliceTextMapper[i]);
        sliceTextActor[i]->SetPosition(90, 10);

        riw[i]->GetRenderer()->AddActor2D(sliceTextActor[i]);

        //设置vtkImagePlaneWidget
        m_planeWidget[i] = vtkSmartPointer<vtkImagePlaneWidget>::New();
        m_planeWidget[i]->SetInteractor(ui->openGLWidget_4->interactor());
        m_planeWidget[i]->SetPicker(picker);
        m_planeWidget[i]->RestrictPlaneToVolumeOn();
        double color[3] = { 0, 0, 0 };
        color[i] = 1;
        m_planeWidget[i]->GetPlaneProperty()->SetColor(color);
        //m_planeWidget[i]->SetTexturePlaneProperty(ipwProp);
        m_planeWidget[i]->TextureInterpolateOff();
        m_planeWidget[i]->SetResliceInterpolateToLinear();
        m_planeWidget[i]->SetInputConnection(m_reader->GetOutputPort());
        m_planeWidget[i]->SetPlaneOrientation(i);
        m_planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
        m_planeWidget[i]->DisplayTextOn();
        m_planeWidget[i]->SetDefaultRenderer(ren);
        m_planeWidget[i]->SetWindowLevel(4096, 2048);
        m_planeWidget[i]->On();
        m_planeWidget[i]->InteractionOn();
    }

    vtkSmartPointer<vtkResliceCursorCallback> cbk = vtkSmartPointer<vtkResliceCursorCallback>::New();

    for (int i = 0; i < 3; i++)
    {
        cbk->IPW[i] = m_planeWidget[i];
        cbk->RCW[i] = riw[i]->GetResliceCursorWidget();
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);
        riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::WindowLevelEvent, cbk);
        //riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResliceThicknessChangedEvent, cbk);
        //riw[i]->GetResliceCursorWidget()->AddObserver(vtkResliceCursorWidget::ResetCursorEvent, cbk);
        //riw[i]->GetInteractorStyle()->AddObserver(vtkCommand::WindowLevelEvent, cbk);
        //riw[i]->AddObserver(vtkResliceImageViewer::SliceChangedEvent, cbk);

        // Make them all share the same color map.
        riw[i]->SetLookupTable(riw[0]->GetLookupTable());
        m_planeWidget[i]->GetColorMap()->SetLookupTable(riw[0]->GetLookupTable());
        // planeWidget[i]->GetColorMap()->SetInput(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap()->GetInput());
        m_planeWidget[i]->SetColorMap(riw[i]->GetResliceCursorWidget()->GetResliceCursorRepresentation()->GetColorMap());
        riw[i]->SetResliceMode(1);//mode ? 1 : 0

        //将十字轴设为透明
        vtkResliceCursorLineRepresentation* rep = dynamic_cast<vtkResliceCursorLineRepresentation*>(riw[i]->GetResliceCursorWidget()->GetRepresentation());
        for (int i = 0; i < 3; i++) rep->GetResliceCursorActor()->GetCenterlineProperty(i)->SetOpacity(0.0);

        //riw[i]->GetRenderer()->GetActiveCamera()->SetParallelProjection(1);
        //riw[i]->GetRenderer()->GetActiveCamera()->SetParallelScale(85);
        riw[i]->GetRenderer()->ResetCamera();
        riw[i]->Render();
    }

    ren->Render();

    ui->openGLWidget_1->show();
    ui->openGLWidget_2->show();
    ui->openGLWidget_3->show();
}





void MPRViewer::constructCPR()
{
    vtkContourWidget* wid = vtkContourWidget::New();
    wid->SetInteractor(riw[2]->GetInteractor());
    wid->SetEnabled(true);
    wid->CreateDefaultRepresentation();
    wid->On();
    //auto rep = dynamic_cast<vtkOrientedGlyphContourRepresentation*>(wid->GetContourRepresentation());
    //rep->GetLinesProperty()->SetColor(1, 0, 0);

    //if (rep) {
    //    vtkResliceCursorLineRepresentation* lineRep = dynamic_cast<vtkResliceCursorLineRepresentation*>(
    //        riw[2]->GetResliceCursorWidget()->GetRepresentation());
    //    vtkMatrix4x4* sourceMatrix = lineRep->GetResliceAxes();
    //    qint32 n = rep->GetNumberOfNodes();
    //    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    //    vtkSmartPointer<vtkCellArray> line = vtkSmartPointer<vtkCellArray>::New();
    //    
    //    for (qint32 i = 0; i < n; ++i) {
    //        double p[3];
    //        rep->GetNthNodeWorldPosition(i, p);
    //        vtkNew<vtkTransform> transform1;
    //        transform1->SetMatrix(sourceMatrix);
    //        transform1->Translate(p[0], p[1], 0);
    //        //qDebug() << i
    //        //    << transform1->GetMatrix()->GetElement(0, 3)
    //        //    << transform1->GetMatrix()->GetElement(1, 3)
    //        //    << transform1->GetMatrix()->GetElement(2, 3);
    //        points->InsertNextPoint(transform1->GetMatrix()->GetElement(0, 3), 
    //            transform1->GetMatrix()->GetElement(1, 3), transform1->GetMatrix()->GetElement(2, 3));
    //        line->InsertCellPoint(i);
    //    }

    //    vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
    //    poly->SetPoints(points);
    //    poly->SetLines(line);

    //    vtkNew<vtkSplineFilter> spline_filter;
    //    spline_filter->SetSubdivideToLength();
    //    spline_filter->SetLength(0.2);
    //    spline_filter->SetInputData(poly);
    //    spline_filter->Update();
    //}
}

void MPRViewer::constructMPR2()
{

}