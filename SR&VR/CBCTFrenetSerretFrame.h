#include "vtkPoints.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"

/**
 * @brief The CBCTFrenetSerretFrame class
 * 弗莱纳公式（Frenet–Serret formulas）
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
    // 计算导数
    void ComputeTangentVectors(vtkIdType pointIdNext,
        vtkIdType pointIdLast,
        double* tangent);
    // 计算二阶导数
    void ComputeNormalVectors(double* tgNext,
        double* tgLast,
        double* normal);
    // 由切线定义的平面上最后法线的投影
    void ComputeConsistentNormalVectors(double* tangent,
        double* lastNormal,
        double* normal);
private:
    CBCTFrenetSerretFrame(const CBCTFrenetSerretFrame&);
    void operator=(const CBCTFrenetSerretFrame&);
    double view_up_;
    int consistent_normals_;
};
