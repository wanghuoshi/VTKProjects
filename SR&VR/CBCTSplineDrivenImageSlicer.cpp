#include "vtkInclude.h"
#include "CBCTSplineDrivenImageSlicer.h"

vtkStandardNewMacro(CBCTSplineDrivenImageSlicer);

CBCTSplineDrivenImageSlicer::CBCTSplineDrivenImageSlicer() {
    this->local_frenetFrames_ = CBCTFrenetSerretFrame::New();
    this->reslicer_ = vtkImageReslice::New();
    this->slice_extent_[0] = 256;
    this->slice_extent_[1] = 256;
    this->slice_spacing_[0] = 0.2;
    this->slice_spacing_[1] = 0.2;
    this->slice_thickness_ = 0.2;
    this->offset_point_ = 0;
    this->offset_line_ = 0;
    this->incidence_ = 0;
    this->probe_input_ = 1;
    this->SetNumberOfInputPorts(2);
    this->SetNumberOfOutputPorts(2);
    this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS,
        vtkDataSetAttributes::SCALARS);
}

CBCTSplineDrivenImageSlicer::~CBCTSplineDrivenImageSlicer() {
    this->local_frenetFrames_->Delete();
    this->reslicer_->Delete();
}


void CBCTSplineDrivenImageSlicer::SetPathConnection(int id, vtkAlgorithmOutput* algOutput) {
    if (id < 0) {
        vtkErrorMacro("Bad index " << id << " for source.");
        return;
    }
    int numConnections = this->GetNumberOfInputConnections(1);
    if (id < numConnections) {
        this->SetNthInputConnection(1, id, algOutput);
    }
    else if (id == numConnections && algOutput) {
        this->AddInputConnection(1, algOutput);
    }
    else if (algOutput) {
        vtkWarningMacro("The source id provided is larger than the maximum "
            "source id, using " << numConnections << " instead.");
        this->AddInputConnection(1, algOutput);
    }
}

void CBCTSplineDrivenImageSlicer::SetPathConnection(vtkAlgorithmOutput* algOutput) {
    this->SetPathConnection(0, algOutput);
}

vtkAlgorithmOutput* CBCTSplineDrivenImageSlicer::GetPathConnection() {
    return(this->GetInputConnection(1, 0));
}

int CBCTSplineDrivenImageSlicer::FillInputPortInformation(
    int port, vtkInformation* info) {
    if (port == 0) {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData"); //CT数据
    }
    else {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData"); //切割线2
    }
    return 1;
}


int CBCTSplineDrivenImageSlicer::FillOutputPortInformation(
    int port, vtkInformation* info) {
    if (port == 0) {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkImageData");
    }
    if (port == 1) {
        info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPolyData");
    }
    return 1;
}


int CBCTSplineDrivenImageSlicer::RequestInformation(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) {
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    int extent[6] = { 0, this->slice_extent_[0] - 1,
                     0, this->slice_extent_[1] - 1,
                     0, 1
    };
    double spacing[3] = { this->slice_spacing_[0], this->slice_spacing_[1], this->slice_thickness_ };
    outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent, 6);
    return 1;
}

int CBCTSplineDrivenImageSlicer::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) {
    // 获取信息对象
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* pathInfo = inputVector[1]->GetInformationObject(0);
    vtkInformation* outImageInfo = outputVector->GetInformationObject(0);
    vtkInformation* outPlaneInfo = outputVector->GetInformationObject(1);
    // 获取输入和输出
    vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkImageData* inputCopy = vtkImageData::New();
    inputCopy->ShallowCopy(input);
    vtkPolyData* inputPath = vtkPolyData::SafeDownCast(pathInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkImageData* outputImage = vtkImageData::SafeDownCast(outImageInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkPolyData* outputPlane = vtkPolyData::SafeDownCast(outPlaneInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkSmartPointer<vtkPolyData> pathCopy = vtkSmartPointer<vtkPolyData>::New();
    pathCopy->ShallowCopy(inputPath);
    // 计算路径的局部法线和切线
    this->local_frenetFrames_->SetInputData(pathCopy);
    this->local_frenetFrames_->Setview_up_(this->incidence_);
    this->local_frenetFrames_->Update();
    // 路径将包含点数据数组“切线”和“向量”
    vtkPolyData* path = static_cast<vtkPolyData*>(this->local_frenetFrames_->GetOutputDataObject(0));
    // 计算单元格中使用了多少个点。如果循环，点可以使用多次
    // 不使用NumberOfPoints，因为我们只需要线条和点
    vtkCellArray* lines = path->GetLines();
    lines->InitTraversal();
    vtkIdType nbCellPoints;
    const vtkIdType* points;
    vtkIdType cellId = -1;
    do {
        lines->GetNextCell(nbCellPoints, points);
        cellId++;
    } while (cellId != this->offset_line_);
    vtkIdType ptId = this->offset_point_;
    if (ptId >= nbCellPoints) {
        ptId = nbCellPoints - 1;
    }
    // 建立一个新的reslicer与图像输入作为输入。
    this->reslicer_->SetInputData(inputCopy);
    // 获取ptId点的Frenet-Serret图表：
    // - position (center)
    // - tangent T
    // - normal N
    double center[3];
    path->GetPoints()->GetPoint(ptId, center);
    vtkDoubleArray* pathTangents = static_cast<vtkDoubleArray*>
        (path->GetPointData()->GetArray("FSTangents"));
    double tangent[3];
    pathTangents->GetTuple(ptId, tangent);
    vtkDoubleArray* pathNormals = static_cast<vtkDoubleArray*>
        (path->GetPointData()->GetArray("FSNormals"));
    double normal[3];
    pathNormals->GetTuple(ptId, normal);
    // Frenet-Serret 图表由 T, N and B = T ^ N
    double crossProduct[3];
    vtkMath::Cross(tangent, normal, crossProduct);
    // 构建平面输出，该输出将表示三维视图中的切片位置
    //vtkSmartPointer<vtkPlaneSource> plane
    //    = vtkSmartPointer<vtkPlaneSource>::New();
    //double planeOrigin[3];
    //double planePoint1[3];
    //double planePoint2[3];
    //for (int comp = 0; comp < 3; comp++) {
    //    planeOrigin[comp] = center[comp] - normal[comp] * this->slice_extent_[1] * this->slice_spacing_[1] / 2.0
    //        - crossProduct[comp] * this->slice_extent_[0] * this->slice_spacing_[0] / 2.0;
    //    planePoint1[comp] = planeOrigin[comp] + crossProduct[comp] * this->slice_extent_[0] * this->slice_spacing_[0];
    //    planePoint2[comp] = planeOrigin[comp] + normal[comp] * this->slice_extent_[1] * this->slice_spacing_[1];
    //}
    //plane->SetOrigin(planeOrigin);
    //plane->SetPoint1(planePoint1);
    //plane->SetPoint2(planePoint2);
    //plane->SetResolution(this->slice_extent_[0],
    //    this->slice_extent_[1]);
    //plane->Update();
    //if (this->probe_input_ == 1) {
    //    vtkSmartPointer<vtkProbeFilter> probe = vtkSmartPointer<vtkProbeFilter>::New();
    //    probe->SetInputConnection(plane->GetOutputPort());
    //    probe->SetSourceData(inputCopy);
    //    probe->Update();
    //    outputPlane->DeepCopy(probe->GetOutputDataObject(0));
    //}
    //else {
    //    outputPlane->DeepCopy(plane->GetOutputDataObject(0));
    //}
    // 构建转换矩阵
    vtkMatrix4x4* resliceAxes = vtkMatrix4x4::New();
    resliceAxes->Identity();
    double origin[4];
    // 仿照 vtkImageReslice:
    // - 1st column contains the resliced image x-axis
    // - 2nd column contains the resliced image y-axis
    // - 3rd column contains the normal of the resliced image plane
    // -> 1st column is normal to the path
    // -> 3nd column is tangent to the path
    // -> 2nd column is B = T^N
    for (int comp = 0; comp < 3; comp++) {
        resliceAxes->SetElement(0, comp, crossProduct[comp]);
        resliceAxes->SetElement(1, comp, normal[comp]);
        resliceAxes->SetElement(2, comp, tangent[comp]);
        origin[comp] = center[comp] -
            normal[comp] * this->slice_extent_[1] * this->slice_spacing_[1] / 2.0 -
            crossProduct[comp] * this->slice_extent_[0] * this->slice_spacing_[0] / 2.0;
    }
    origin[3] = 1.0;
    double originXYZW[4];
    resliceAxes->MultiplyPoint(origin, originXYZW);
    resliceAxes->Transpose();
    double neworiginXYZW[4];
    resliceAxes->MultiplyPoint(originXYZW, neworiginXYZW);
    resliceAxes->SetElement(0, 3, neworiginXYZW[0]);
    resliceAxes->SetElement(1, 3, neworiginXYZW[1]);
    resliceAxes->SetElement(2, 3, neworiginXYZW[2]);
    this->reslicer_->SetResliceAxes(resliceAxes);
    this->reslicer_->SetInformationInput(input);
    this->reslicer_->SetInterpolationModeToCubic();
    this->reslicer_->SetOutputDimensionality(2);
    this->reslicer_->SetOutputOrigin(0, 0, 0);
    this->reslicer_->SetOutputExtent(0, this->slice_extent_[0] - 1, 0, this->slice_extent_[1] - 1, 0, 1);
    this->reslicer_->SetOutputSpacing(this->slice_spacing_[0], this->slice_spacing_[1], this->slice_thickness_);
    this->reslicer_->Update();
    resliceAxes->Delete();
    outputImage->DeepCopy(this->reslicer_->GetOutputDataObject(0));
    outputImage->GetPointData()->GetScalars()->SetName("ReslicedImage");
    return 1;
}