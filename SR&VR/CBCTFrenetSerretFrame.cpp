#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkMath.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "CBCTFrenetSerretFrame.h"

vtkStandardNewMacro(CBCTFrenetSerretFrame);

CBCTFrenetSerretFrame::CBCTFrenetSerretFrame() {
    this->consistent_normals_ = 0;
    this->view_up_ = 0;
}

CBCTFrenetSerretFrame::~CBCTFrenetSerretFrame() {
}

int CBCTFrenetSerretFrame::FillInputPortInformation(int port, vtkInformation* info) {
    if (port == 0) {
        info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
    }
    return 1;
}

int CBCTFrenetSerretFrame::RequestData(
    vtkInformation* vtkNotUsed(request),
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);
    vtkPolyData* input = vtkPolyData::SafeDownCast(
        inInfo->Get(vtkDataObject::DATA_OBJECT()));
    vtkPolyData* output = vtkPolyData::SafeDownCast(
        outInfo->Get(vtkDataObject::DATA_OBJECT()));
    output->DeepCopy(input);
    vtkDoubleArray* tangents = vtkDoubleArray::New(); //����
    tangents->SetNumberOfComponents(3);
    tangents->SetNumberOfTuples(input->GetNumberOfPoints());
    tangents->SetName("FSTangents");
    vtkDoubleArray* normals = vtkDoubleArray::New(); //������
    normals->SetNumberOfComponents(3);
    normals->SetNumberOfTuples(input->GetNumberOfPoints());
    normals->SetName("FSNormals");
    vtkCellArray* lines = output->GetLines();
    lines->InitTraversal();
    vtkIdType nbPoints;
    const vtkIdType* points;
    int cellIdx;
    for (cellIdx = 0; cellIdx < lines->GetNumberOfCells(); cellIdx++) {
        lines->GetNextCell(nbPoints, points);
        for (int i = 0; i < nbPoints; i++) {
            double tangent[3];
            if (i == 0) {
                this->ComputeTangentVectors(points[0], points[1], tangent);
            }
            else if (i == nbPoints - 1) {
                this->ComputeTangentVectors(points[nbPoints - 2], points[nbPoints - 1], tangent);
            }
            else {
                this->ComputeTangentVectors(points[i - 1], points[i + 1], tangent);
            }
            vtkMath::Normalize(tangent);
            tangents->SetTuple(points[i], tangent);
        }
        for (int i = 0; i < nbPoints; i++) {
            if (!this->consistent_normals_ || i == 0) {
                double tangentLast[3], tangentNext[3], normal[3];
                if (i == 0) {
                    tangents->GetTuple(points[i], tangentLast);
                }
                else {
                    tangents->GetTuple(points[i - 1], tangentLast);
                }
                if (i == nbPoints - 1) {
                    tangents->GetTuple(points[i], tangentNext);
                }
                else {
                    tangents->GetTuple(points[i + 1], tangentNext);
                }
                this->ComputeNormalVectors(tangentLast, tangentNext, normal);
                if (this->consistent_normals_) {
                    this->RotateVector(normal, tangentLast, this->view_up_);
                }
                vtkMath::Normalize(normal);
                normals->SetTuple(points[i], normal);
            }
            if (this->consistent_normals_ && i != 0) {
                double tangent[3], lastNormal[3], normal[3];
                normals->GetTuple(points[i - 1], lastNormal);
                tangents->GetTuple(points[i], tangent);
                this->ComputeConsistentNormalVectors(tangent,
                    lastNormal,
                    normal);
                vtkMath::Normalize(normal);
                normals->SetTuple(points[i], normal);
            }
        }
    }
    output->GetPointData()->AddArray(normals);
    output->GetPointData()->AddArray(tangents);
    normals->Delete();
    tangents->Delete();
    return 1;
}

// ����������ĵ���
void CBCTFrenetSerretFrame::ComputeTangentVectors(
    vtkIdType pointIdNext, vtkIdType pointIdLast, double* tangent) {
    vtkPolyData* input = static_cast<vtkPolyData*>(this->GetInput(0));
    double ptNext[3];
    double ptLast[3];
    input->GetPoint(pointIdNext, ptNext);
    input->GetPoint(pointIdLast, ptLast);
    int comp;
    for (comp = 0; comp < 3; comp++) {
        tangent[comp] = (ptLast[comp] - ptNext[comp]) / 2;
    }
}

// �����߶����ƽ��������ߵ�ͶӰ
void CBCTFrenetSerretFrame::ComputeConsistentNormalVectors(double* tangent,
    double* normalLast,
    double* normal) {
    double temp[3];
    vtkMath::Cross(normalLast, tangent, temp); //��������
    vtkMath::Cross(tangent, temp, normal);
}

// ������׵���
void CBCTFrenetSerretFrame::ComputeNormalVectors(double* tgNext,
    double* tgLast,
    double* normal) {
    int comp;
    for (comp = 0; comp < 3; comp++) {
        normal[comp] = (tgNext[comp] - tgLast[comp]);
    }
    if (vtkMath::Norm(normal) == 0) {
        double unit[3] = { 1, 0, 0 };
        vtkMath::Cross(tgLast, unit, normal);
    }
}

void CBCTFrenetSerretFrame::RotateVector(double* vector, const double* axis, double angle) {
    double UdotN = vtkMath::Dot(vector, axis);
    double NvectU[3];
    vtkMath::Cross(axis, vector, NvectU);
    for (int comp = 0; comp < 3; comp++) {
        vector[comp] = cos(angle) * vector[comp]
            + (1 - cos(angle)) * UdotN * axis[comp]
            + sin(angle) * NvectU[comp];
    }
}