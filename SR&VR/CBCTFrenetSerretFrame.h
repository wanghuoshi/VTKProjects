#include "vtkPoints.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"

/**
 * @brief The CBCTFrenetSerretFrame class
 * �����ɹ�ʽ��Frenet�CSerret formulas��
 */
class CBCTFrenetSerretFrame : public vtkPolyDataAlgorithm {
public:
    vtkTypeMacro(CBCTFrenetSerretFrame, vtkPolyDataAlgorithm)
        static CBCTFrenetSerretFrame* New();
    vtkSetMacro(view_up_, double)
        vtkGetMacro(view_up_, double)
        static void RotateVector(double* vector, const double* axis, double angle);
protected:
    CBCTFrenetSerretFrame();
    ~CBCTFrenetSerretFrame() override;
    virtual int RequestData(
        vtkInformation*, vtkInformationVector**, vtkInformationVector*)override;
    virtual int FillInputPortInformation(int port, vtkInformation* info)override;
    // ���㵼��
    void ComputeTangentVectors(vtkIdType pointIdNext,
        vtkIdType pointIdLast,
        double* tangent);
    // ������׵���
    void ComputeNormalVectors(double* tgNext,
        double* tgLast,
        double* normal);
    // �����߶����ƽ��������ߵ�ͶӰ
    void ComputeConsistentNormalVectors(double* tangent,
        double* lastNormal,
        double* normal);
private:
    CBCTFrenetSerretFrame(const CBCTFrenetSerretFrame&);
    void operator=(const CBCTFrenetSerretFrame&);
    double view_up_;
    int consistent_normals_;
};
