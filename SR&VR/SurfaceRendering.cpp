#include "SurfaceRendering.h"
#include "CBCTSplineDrivenImageSlicer.h"
#include <QMessageBox>

class vtkMyContourCallback : public vtkCommand
{
public:
    static vtkMyContourCallback* New() { return new vtkMyContourCallback; };

    void Execute(vtkObject* caller, unsigned long ev, void* callData) override
    {
        if (ev == vtkCommand::EndInteractionEvent)
        {
            auto widget = dynamic_cast<vtkContourWidget*>(caller);
            auto rep = dynamic_cast<vtkOrientedGlyphContourRepresentation*>(widget->GetContourRepresentation());
            rep->GetLinesProperty()->SetColor(1, 0, 0);

            if (rep) { 
                qint32 n = rep->GetNumberOfNodes();
                vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
                vtkSmartPointer<vtkCellArray> line = vtkSmartPointer<vtkCellArray>::New();
                
                for (qint32 i = 0; i < n; ++i) {
                    double p[3];
                    rep->GetNthNodeWorldPosition(i, p);
                    vtkNew<vtkTransform> transform1;
                    transform1->SetMatrix(axialResliceMatrix);
                    transform1->Translate(p[0], p[1], 0);
                    printf("%d %f %f %f\n",i,transform1->GetMatrix()->GetElement(0, 3),
                        transform1->GetMatrix()->GetElement(1, 3),
                        transform1->GetMatrix()->GetElement(2, 3));
                    points->InsertNextPoint(transform1->GetMatrix()->GetElement(0, 3), 
                        transform1->GetMatrix()->GetElement(1, 3), transform1->GetMatrix()->GetElement(2, 3));
                    line->InsertCellPoint(i);
                }

                // Create a cell array to connect the points into meaningful geometry
                vtkIdType* vertexIndices = new vtkIdType[n];
                for (int i = 0; i < n; i++) { vertexIndices[i] = static_cast<vtkIdType>(i); }
                // Set the last vertex to 0; this means the last line segment will join the 19th point (vertices[19])
                // with the first one (vertices[0]), thus closing the circle.
                //vertexIndices[numPts] = 0;
                line->InsertNextCell(n, vertexIndices);

                vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
                poly->SetPoints(points);
                poly->SetLines(line);
                
                vtkNew<vtkSplineFilter> spline_filter;
                spline_filter->SetSubdivideToLength();
                spline_filter->SetLength(1);
                spline_filter->SetInputData(poly);
                spline_filter->Update();
 
                vtkNew<vtkImageAppend> append;
                append->SetAppendAxis(2);

                vtkNew<CBCTSplineDrivenImageSlicer> reslicer;
                reslicer->SetInputData(reader->GetOutput());
                reslicer->SetPathConnection(spline_filter->GetOutputPort());
                long long nb_points = spline_filter->GetOutput()->GetNumberOfPoints(); printf("%d...........", nb_points);
                for (int pt_id = 0; pt_id < nb_points; pt_id++) {
                    reslicer->Setoffset_point_(pt_id); 
                    reslicer->Update();
                    vtkNew<vtkImageData> tempSlice;
                    tempSlice->DeepCopy(reslicer->GetOutput());
                    append->AddInputData(tempSlice);
                }
                append->Update();

                vtkNew<vtkImagePermute> permute_filter;
                permute_filter->SetInputData(append->GetOutput());
                permute_filter->SetFilteredAxes(2, 0, 1);
                permute_filter->Update();

                vtkNew<vtkImageFlip> flip_filter;
                flip_filter->SetInputData(permute_filter->GetOutput());
                flip_filter->SetFilteredAxes(1);
                flip_filter->Update();

                viewer->SetInputData(flip_filter->GetOutput());
            }
        }
    }

    vtkMatrix4x4* axialResliceMatrix;
    vtkDICOMReader* reader;
    vtkImageActor* viewer;
};

SurfaceRendering::SurfaceRendering(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    ui.mcValue->setMinimum(0);
    ui.mcValue->setMaximum(1500);
    ui.mcValue->setValue(1200);
    connect(ui.mcValue, &QSlider::valueChanged, this, &SurfaceRendering::mcValueChanged);
    connect(ui.mcBtn, &QPushButton::clicked, this, &SurfaceRendering::mc);
    connect(ui.dcBtn, &QPushButton::clicked, this, &SurfaceRendering::dc);
    connect(ui.rayCasting, &QPushButton::clicked, this, &SurfaceRendering::rayCasting);
    connect(ui.textureMapping, &QPushButton::clicked, this, &SurfaceRendering::textureMapping);
    connect(ui.MIP, &QPushButton::clicked, this, &SurfaceRendering::MIP);
    connect(ui.cprBtn, &QPushButton::clicked, this, &SurfaceRendering::cpr);

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
}

SurfaceRendering::~SurfaceRendering()
{}

void SurfaceRendering::mcValueChanged(int value)
{
    boneExtractor->SetValue(0, value);
    boneExtractor->Update();
    ui.openGLWidget->renderWindow()->Render();
}

void SurfaceRendering::mc()
{
    boneExtractor = vtkSmartPointer< vtkMarchingCubes >::New();
    boneExtractor->SetInputConnection(m_reader->GetOutputPort());
    boneExtractor->ComputeNormalsOn();
    boneExtractor->SetValue(-1, 1200); //设置提取的等值信息

    //剔除旧的或废除的数据单元，提高绘制速度(可略去这一步)
    vtkSmartPointer<vtkStripper> boneStripper = vtkSmartPointer< vtkStripper >::New(); //三角带连接
    boneStripper->SetInputConnection(boneExtractor->GetOutputPort());

    //平滑滤波
    vtkSmartPointer<vtkSmoothPolyDataFilter> pSmoothPolyDataFilter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
    pSmoothPolyDataFilter->SetInputConnection(boneStripper->GetOutputPort());
    //pSmoothPolyDataFilter->SetNumberOfIterations(m_nNumberOfIterations);
    pSmoothPolyDataFilter->SetRelaxationFactor(0.05);

    vtkSmartPointer<vtkPolyDataNormals> pPolyDataNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
    pPolyDataNormals->SetInputConnection(pSmoothPolyDataFilter->GetOutputPort());
    //pPolyDataNormals->SetFeatureAngle(m_nFeatureAngle);

    //建立映射
    vtkSmartPointer< vtkPolyDataMapper > surfMapper = vtkSmartPointer< vtkPolyDataMapper >::New();
    surfMapper->SetInputConnection(boneExtractor->GetOutputPort());
    surfMapper->ScalarVisibilityOn();

    //建立角色
    vtkSmartPointer< vtkActor > surfActor = vtkSmartPointer< vtkActor >::New();
    surfActor->SetMapper(surfMapper);
    surfActor->GetProperty()->SetDiffuseColor(1.0, 1.0, 1.0);
    surfActor->GetProperty()->SetSpecular(.3);
    surfActor->GetProperty()->SetSpecularPower(20);

    //定义绘制器
    vtkSmartPointer< vtkRenderer > renderer = vtkSmartPointer< vtkRenderer >::New();
    renderer->AddActor(surfActor);
    renderer->SetBackground(1.0, 1.0, 1.0);

    //定义绘制窗口
    vtkSmartPointer< vtkGenericOpenGLRenderWindow > renWin = vtkSmartPointer< vtkGenericOpenGLRenderWindow >::New();
    renWin->AddRenderer(renderer);

    ui.openGLWidget->setRenderWindow(renWin);
    ui.openGLWidget->renderWindow()->Render();
}

void SurfaceRendering::dc()
{
    //DC算法   耗时长，模型有明显缝隙
    vtkSmartPointer< vtkRecursiveDividingCubes > boneExtractor = vtkSmartPointer< vtkRecursiveDividingCubes >::New();
    boneExtractor->SetInputConnection(m_reader->GetOutputPort());
    boneExtractor->SetValue(1000);
    boneExtractor->SetDistance(1);
    boneExtractor->SetIncrement(2);
    boneExtractor->Update();

    //剔除旧的或废除的数据单元，提高绘制速度(可略去这一步)
    vtkSmartPointer<vtkStripper> boneStripper = vtkSmartPointer< vtkStripper >::New(); //三角带连接
    boneStripper->SetInputConnection(boneExtractor->GetOutputPort());

    //平滑滤波
    vtkSmartPointer<vtkSmoothPolyDataFilter> pSmoothPolyDataFilter = vtkSmartPointer<vtkSmoothPolyDataFilter>::New();
    pSmoothPolyDataFilter->SetInputConnection(boneStripper->GetOutputPort());
    //pSmoothPolyDataFilter->SetNumberOfIterations(m_nNumberOfIterations);
    pSmoothPolyDataFilter->SetRelaxationFactor(0.05);

    vtkSmartPointer<vtkPolyDataNormals> pPolyDataNormals = vtkSmartPointer<vtkPolyDataNormals>::New();
    pPolyDataNormals->SetInputConnection(pSmoothPolyDataFilter->GetOutputPort());
    //pPolyDataNormals->SetFeatureAngle(m_nFeatureAngle);

    //建立映射
    vtkSmartPointer< vtkPolyDataMapper > surfMapper = vtkSmartPointer< vtkPolyDataMapper >::New();
    surfMapper->SetInputConnection(boneExtractor->GetOutputPort());
    surfMapper->ScalarVisibilityOn();

    //建立角色
    vtkSmartPointer< vtkActor > surfActor = vtkSmartPointer< vtkActor >::New();
    surfActor->SetMapper(surfMapper);
    surfActor->GetProperty()->SetDiffuseColor(1.0, 1.0, 1.0);
    surfActor->GetProperty()->SetSpecular(.3);
    surfActor->GetProperty()->SetSpecularPower(20);

    //定义绘制器
    vtkSmartPointer< vtkRenderer > renderer = vtkSmartPointer< vtkRenderer >::New();
    renderer->AddActor(surfActor);
    renderer->SetBackground(1.0, 1.0, 1.0);

    //定义绘制窗口
    vtkSmartPointer< vtkGenericOpenGLRenderWindow > renWin = vtkSmartPointer< vtkGenericOpenGLRenderWindow >::New();
    renWin->AddRenderer(renderer);

    ui.openGLWidget->setRenderWindow(renWin);
    ui.openGLWidget->renderWindow()->Render();
}

void SurfaceRendering::rayCasting()
{
    double iso1 = 500.0;
    double iso2 = 1150.0;

    auto colors = vtkSmartPointer<vtkNamedColors>::New();

    vtkSmartPointer<vtkImageGaussianSmooth> gaussianSmoothFilter = vtkSmartPointer<vtkImageGaussianSmooth>::New();
    gaussianSmoothFilter->SetInputConnection(m_reader->GetOutputPort());
    gaussianSmoothFilter->SetDimensionality(3);
    gaussianSmoothFilter->SetRadiusFactor(3);
    gaussianSmoothFilter->SetStandardDeviation(2);
    gaussianSmoothFilter->Update();

    vtkSmartPointer<vtkGPUVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkGPUVolumeRayCastMapper>::New();
    volumeMapper->SetInputConnection(gaussianSmoothFilter->GetOutputPort());
    volumeMapper->AutoAdjustSampleDistancesOff();
    volumeMapper->SetSampleDistance(volumeMapper->GetSampleDistance() / 6);	//设置光线采样距离
    //volumeMapper->SetBlendModeToIsoSurface();
    //volumeMapper->SetAutoAdjustSampleDistances(0);//设置图像采样步长
    //volumeMapper->SetImageSampleDistance(4);

    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    //volumeProperty->SetInterpolationTypeToLinear();
    //volumeProperty->ShadeOn();  //打开或者关闭阴影测试
    //volumeProperty->SetAmbient(0.4);
    //volumeProperty->SetDiffuse(0.6);  //漫反射
    //volumeProperty->SetSpecular(0.2); //镜面反射
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->ShadeOn();  //打开或者关闭阴影测试
    volumeProperty->SetAmbient(0.3);
    volumeProperty->SetDiffuse(0.8);  //漫反射
    volumeProperty->SetSpecular(0.2); //镜面反射
    //设置不透明度
    //vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    //compositeOpacity->AddPoint(50, 1);
    //compositeOpacity->AddPoint(750, 0.1);
    //compositeOpacity->AddPoint(1500, 1);
    vtkSmartPointer<vtkPiecewiseFunction> opacityFun = vtkSmartPointer<vtkPiecewiseFunction>::New();
    opacityFun->AddPoint(-1100, 0);
    opacityFun->AddPoint(1043, 0);
    opacityFun->AddPoint(1459, 1);
    opacityFun->AddPoint(3072, 1);
    opacityFun->AddPoint(3899, 1);
    volumeProperty->SetScalarOpacity(opacityFun); //设置不透明度传输函数

    //设置梯度不透明属性
    //vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    //volumeGradientOpacity->AddPoint(10, 0.0);
    //volumeGradientOpacity->AddPoint(90, 0.5);
    //volumeGradientOpacity->AddPoint(100, 1.0);
    //volumeProperty->SetGradientOpacity(volumeGradientOpacity);//设置梯度不透明度效果对比
    //auto scalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    //scalarOpacity->AddPoint(iso1, .3);
    //scalarOpacity->AddPoint(iso2, 0.6);
    
    //设置颜色属性
    //vtkSmartPointer<vtkColorTransferFunction> color = vtkSmartPointer<vtkColorTransferFunction>::New();
    //color->AddRGBPoint(0.000, 0.00, 0.00, 0.00);
    //color->AddRGBPoint(64.00, 1.00, 0.52, 0.30);
    //color->AddRGBPoint(190.0, 1.00, 1.00, 1.00);
    //color->AddRGBPoint(220.0, 0.20, 0.20, 0.20);
    //volumeProperty->SetColor(color);
    /*auto colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
    colorTransferFunction->RemoveAllPoints();
    colorTransferFunction->AddRGBPoint(
        iso2,
        colors->GetColor3d("ivory").GetData()[0],
        colors->GetColor3d("ivory").GetData()[1],
        colors->GetColor3d("ivory").GetData()[2]);
    colorTransferFunction->AddRGBPoint(
        iso1,
        colors->GetColor3d("flesh").GetData()[0],
        colors->GetColor3d("flesh").GetData()[1],
        colors->GetColor3d("flesh").GetData()[2]);
        */
    vtkSmartPointer<vtkColorTransferFunction> colorFun = vtkSmartPointer<vtkColorTransferFunction>::New();
    colorFun->AddRGBPoint(-1100, 0, 0, 0);
    colorFun->AddRGBPoint(129, 0.3, 0, 0);
    colorFun->AddRGBPoint(1059, 0.8, 0.07, 0);
    colorFun->AddRGBPoint(1459, 1, 0.9, 0.6);
    colorFun->AddRGBPoint(1863, 1, 1, 1);
    colorFun->AddRGBPoint(3899, 1, 1, 1);
    volumeProperty->SetColor(colorFun);
    //volumeProperty->SetScalarOpacity(scalarOpacity);

    vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
    ren->SetBackground(0, 0, 0);
    ren->AddVolume(volume);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> rw = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    rw->AddRenderer(ren);
    //rw->SetSize(640, 480);
    rw->Render();
    rw->SetWindowName("VolumeRendering");

    ui.openGLWidget->setRenderWindow(rw);
    ui.openGLWidget->renderWindow()->Render();
}

void SurfaceRendering::textureMapping()
{
    double iso1 = 500.0;
    double iso2 = 1150.0;

    auto colors = vtkSmartPointer<vtkNamedColors>::New();

    auto mapper = vtkSmartPointer<vtkOpenGLGPUVolumeRayCastMapper>::New();
    mapper->SetInputConnection(m_reader->GetOutputPort());
    mapper->AutoAdjustSampleDistancesOff();
    mapper->SetSampleDistance(0.5);
    mapper->SetBlendModeToIsoSurface();
    //mapper->SetPartitions(4, 2, 4);//设置分区；注释后为不分区

    auto colorTransferFunction = vtkSmartPointer<vtkColorTransferFunction>::New();
    colorTransferFunction->RemoveAllPoints();
    colorTransferFunction->AddRGBPoint(
        iso2,
        colors->GetColor3d("ivory").GetData()[0],
        colors->GetColor3d("ivory").GetData()[1],
        colors->GetColor3d("ivory").GetData()[2]);
    colorTransferFunction->AddRGBPoint(
        iso1,
        colors->GetColor3d("flesh").GetData()[0],
        colors->GetColor3d("flesh").GetData()[1],
        colors->GetColor3d("flesh").GetData()[2]);

    auto scalarOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    scalarOpacity->AddPoint(iso1, .3);
    scalarOpacity->AddPoint(iso2, 0.6);

    auto volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->ShadeOn();
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->SetColor(colorTransferFunction);
    volumeProperty->SetScalarOpacity(scalarOpacity);

    auto volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(mapper);
    volume->SetProperty(volumeProperty);

    // Add some contour values to draw iso surfaces
    volumeProperty->GetIsoSurfaceValues()->SetValue(0, iso1);
    volumeProperty->GetIsoSurfaceValues()->SetValue(1, iso2);

    // Generate a good view
    vtkSmartPointer<vtkCamera> aCamera = vtkSmartPointer<vtkCamera>::New();
    aCamera->SetViewUp(0, 0, -1);
    aCamera->SetPosition(0, -1, 0);
    aCamera->SetFocalPoint(0, 0, 0);

    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
    //ren->SetBackground(0, 1, 0);
    ren->SetBackground(colors->GetColor3d("cornflower").GetData());
    ren->AddVolume(volume);
    ren->SetActiveCamera(aCamera);
    ren->ResetCamera();
    
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> rw = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    rw->AddRenderer(ren);
    rw->SetSize(640, 480);
    rw->Render();
    rw->SetWindowName("TextureMapping");

    ui.openGLWidget->setRenderWindow(rw);

    aCamera->Azimuth(30.0);
    aCamera->Elevation(30.0);
    aCamera->Dolly(1.5);

    ui.openGLWidget->renderWindow()->Render();
}

void SurfaceRendering::MIP()
{
    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
    volumeMapper->SetInputConnection(m_reader->GetOutputPort());
    volumeMapper->SetBlendModeToAdditive();
    volumeMapper->SetSampleDistance(0.3);
    volumeMapper->AutoAdjustSampleDistancesOff();

    //vtkVolumeProperty光照与阴影
    vtkSmartPointer<vtkVolumeProperty> volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    volumeProperty->SetInterpolationTypeToLinear();
    volumeProperty->ShadeOn();//打开阴影效果
    volumeProperty->SetAmbient(0.2);//设置环境光系数
    volumeProperty->SetDiffuse(1.2);//设置散射光系数
    volumeProperty->SetSpecular(0.1);//设置反射光系数
    volumeProperty->SetSpecularPower(10);//设置高光强度

    //梯度不透明函数
    vtkSmartPointer<vtkPiecewiseFunction> compositeOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    compositeOpacity->AddPoint(70, 0.00);//梯度小于等于70的点的不透明乘子设为0，即完全透明
    compositeOpacity->AddPoint(90, 0.40);//梯度等于90的点的不透明度为0.4；梯度在10~90之间，不透明度通过线性映射至0~0.4之间
    compositeOpacity->AddPoint(180, 0.60);//梯度大于等于180的点的不透明度为1.0；梯度在90~180之间，不透明度通过线性映射至0.4~1.0之间
    volumeProperty->SetScalarOpacity(compositeOpacity); //设置标量场不透明度函数

    vtkSmartPointer<vtkPiecewiseFunction> volumeGradientOpacity = vtkSmartPointer<vtkPiecewiseFunction>::New();
    volumeGradientOpacity->AddPoint(10, 0.0);
    volumeGradientOpacity->AddPoint(90, 0.5);
    volumeGradientOpacity->AddPoint(100, 1.0);
    //volumeProperty->SetGradientOpacity(volumeGradientOpacity);//设置梯度不透明函数

    //颜色传输函数
    vtkSmartPointer<vtkColorTransferFunction> color = vtkSmartPointer<vtkColorTransferFunction>::New();
    //color->AddRGBPoint(0.000, 0.00, 0.00, 0.00);
    color->AddRGBPoint(64.00, 1.00, 0.52, 0.30);
    //color->AddRGBPoint(190.0, 1.00, 1.00, 1.00);
    //color->AddRGBPoint(220.0, 0.20, 0.20, 0.20);
    volumeProperty->SetColor(color);

    vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(volumeProperty);

    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
    ren->SetBackground(1, 1, 1);
    ren->AddVolume(volume);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> rw = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    rw->AddRenderer(ren);
    rw->SetSize(640, 480);
    rw->Render();
    rw->SetWindowName("MIP");

    ui.openGLWidget->setRenderWindow(rw);
    ui.openGLWidget->renderWindow()->Render();
}

void SurfaceRendering::cpr()
{
    int dims[3];
    m_reader->GetOutput()->GetDimensions(dims);
    printf("%d", dims[0]);

    //轴状面的切面坐标系矩阵
    double Axial[16] = {
            1, 0, 0, 0,
            0, 1, 0, 0,
            0, 0, 1, 60,
            0, 0, 0, 1 };
    AxialResliceMatrix = vtkMatrix4x4::New();
    AxialResliceMatrix->DeepCopy(Axial);
    vtkSmartPointer<vtkImageReslice> myImageReslice = vtkSmartPointer<vtkImageReslice>::New();
    //设置体数据来源
    myImageReslice->SetInputData(m_reader->GetOutput());
    //设置vtkImageReslice的切面坐标系矩阵
    myImageReslice->SetResliceAxes(AxialResliceMatrix);
    //设置输出是一个切片，而不是一个卷
    myImageReslice->SetOutputDimensionality(2);
    //设置切面算法的插值方式为线性插值
    myImageReslice->SetInterpolationModeToLinear();

    //设置颜色查找表
    vtkSmartPointer<vtkLookupTable> myLookupTable = vtkSmartPointer<vtkLookupTable>::New();
    myLookupTable->SetRange(0, 4800);
    myLookupTable->SetValueRange(0.0, 1.0);
    myLookupTable->SetSaturationRange(0.0, 0.0);
    myLookupTable->SetRampToLinear();
    myLookupTable->Build();

    //映射到切面图像
    vtkSmartPointer<vtkImageMapToColors> myMapToColors = vtkSmartPointer<vtkImageMapToColors>::New();
    myMapToColors->SetLookupTable(myLookupTable);
    myMapToColors->SetInputConnection(myImageReslice->GetOutputPort());
    myMapToColors->Update();

    //设置连接到图像Actor，此时将myImageActor连接设置好的管道中的vtkRenderer上就完成了
    vtkSmartPointer<vtkImageActor> myImageActor = vtkSmartPointer<vtkImageActor>::New();
    myImageActor->SetInputData(myMapToColors->GetOutput());

    // Create the contour widget
    vtkSmartPointer<vtkContourWidget> contourWidget = vtkSmartPointer<vtkContourWidget>::New();
    // Override the default representation for the contour widget to customize its look
    vtkSmartPointer<vtkOrientedGlyphContourRepresentation> contourRepresentation = vtkSmartPointer<vtkOrientedGlyphContourRepresentation>::New();
    contourRepresentation->GetLinesProperty()->SetColor(1, 0, 0); // Set color to red
    
    contourWidget->SetRepresentation(contourRepresentation);
    
    // Generate a set of points
    int numPts = 5;
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    for (int i = 0; i < numPts; i++)
    {
        // Create numPts points 
        points->InsertPoint(static_cast<vtkIdType>(i), i*160 / numPts, i*120 / numPts, 0.0);
    }

    // Create a cell array to connect the points into meaningful geometry
    vtkIdType* vertexIndices = new vtkIdType[numPts];
    for (int i = 0; i < numPts; i++) { vertexIndices[i] = static_cast<vtkIdType>(i); }
    // Set the last vertex to 0; this means the last line segment will join the 19th point (vertices[19])
    // with the first one (vertices[0]), thus closing the circle.
    //vertexIndices[numPts] = 0;
    vtkSmartPointer<vtkCellArray> lines = vtkSmartPointer<vtkCellArray>::New();
    lines->InsertNextCell(numPts, vertexIndices);

    // Create polydata to hold the geometry just created, and populate it
    vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
    polydata->SetPoints(points);
    polydata->SetLines(lines);

    vtkNew<vtkSplineFilter> spline_filter;
    spline_filter->SetSubdivideToLength();
    spline_filter->SetLength(0.1);
    spline_filter->SetInputData(polydata);
    spline_filter->Update();
    long long nb_points = spline_filter->GetOutput()->GetNumberOfPoints(); printf("*****%d*****", nb_points);

    vtkSmartPointer<vtkRenderer> ren = vtkSmartPointer<vtkRenderer>::New();
    ren->SetBackground(1, 1, 1);
    ren->AddActor(myImageActor);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> rw = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    rw->AddRenderer(ren);
    //rw->SetSize(640, 480);
    rw->FullScreenOn();
    rw->SetWindowName("CPR");

    // Create the events manager for the renderer window
    vtkSmartPointer<vtkRenderWindowInteractor> interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    interactor->SetRenderWindow(rw);

    // Use the "trackball camera" interactor style, rather than the default "joystick camera"
    vtkSmartPointer<vtkInteractorStyleTrackballCamera> style = vtkSmartPointer<vtkInteractorStyleTrackballCamera>::New();
    interactor->SetInteractorStyle(style);

    ui.openGLWidget->setRenderWindow(rw);
    
    // Set up the contour widget within the visualization pipeline just assembled
    contourWidget->SetInteractor(rw->GetInteractor());
    contourWidget->On();							// Turn on the interactor observer
    contourWidget->Initialize(polydata);
    contourRepresentation->SetClosedLoop(0);

    vtkNew<vtkMyContourCallback> cbk;
    vtkSmartPointer<vtkImageActor> viewer = vtkSmartPointer<vtkImageActor>::New();
    cbk->axialResliceMatrix = AxialResliceMatrix;
    cbk->reader = m_reader;
    cbk->viewer = viewer;
    contourWidget->AddObserver(vtkCommand::EndInteractionEvent, cbk);

    ui.openGLWidget->setRenderWindow(rw);
    rw->Render();    
    
    std::cout << "the number of nodes is " << contourRepresentation->GetNumberOfNodes() << std::endl;

    ren->ResetCamera();						// Reposition camera to fit the scene elements

    interactor->Initialize();
    // Start the interaction
    interactor->Start();

    vtkSmartPointer<vtkRenderer> ren2 = vtkSmartPointer<vtkRenderer>::New();
    ren2->SetBackground(1, 0, 0);
    ren2->AddActor(viewer);

    vtkSmartPointer<vtkGenericOpenGLRenderWindow> rw2 = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    rw2->AddRenderer(ren2);
    rw2->SetSize(480, 640);
    ui.openGLWidget_2->setRenderWindow(rw2);
    rw2->SetWindowName("cpr");
    //rw2->Render();
    ren2->ResetCamera();
    ren2->Render();

    //（5）创建交互RenderWindowInteractor以及交互样式
    vtkSmartPointer<vtkRenderWindowInteractor>renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    vtkSmartPointer<vtkInteractorStyleImage> style2 = vtkSmartPointer<vtkInteractorStyleImage>::New();
    renderWindowInteractor->SetInteractorStyle(style2);
    renderWindowInteractor->SetRenderWindow(rw2);
    renderWindowInteractor->Initialize();
    renderWindowInteractor->Start();
}