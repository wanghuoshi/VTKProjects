#include "vtkInclude.h"
#include "CBCTFrenetSerretFrame.h"

/**
 * @brief The CBCTSplineDrivenImageSlicer class
 * �������������и�ͼƬ
 */
class CBCTSplineDrivenImageSlicer : public vtkImageAlgorithm {
public:
    vtkTypeMacro(CBCTSplineDrivenImageSlicer, vtkImageAlgorithm)
        static CBCTSplineDrivenImageSlicer* New();
    void SetPathConnection(int id, vtkAlgorithmOutput* algOutput);
    void SetPathConnection(vtkAlgorithmOutput* algOutput);
    vtkAlgorithmOutput* GetPathConnection();
    vtkSetMacro(offset_point_, vtkIdType)
protected:
    CBCTSplineDrivenImageSlicer();
    ~CBCTSplineDrivenImageSlicer()override;

    virtual int RequestData(vtkInformation*, vtkInformationVector**,
        vtkInformationVector*)override;
    virtual int FillInputPortInformation(int port, vtkInformation* info)override;
    virtual int FillOutputPortInformation(int, vtkInformation*)override;
    virtual int RequestInformation(vtkInformation*, vtkInformationVector**,
        vtkInformationVector*)override;
private:
    CBCTSplineDrivenImageSlicer(const CBCTSplineDrivenImageSlicer&);
    void operator=(const CBCTSplineDrivenImageSlicer&);
    CBCTFrenetSerretFrame* local_frenetFrames_;
    vtkImageReslice* reslicer_;
    int slice_extent_[2]; // ���image��xy��������
    double slice_spacing_[2]; // ���image��xy���
    double slice_thickness_; // ���image��z����
    double incidence_;// ��ʼ����������ת
    vtkIdType offset_point_;
    vtkIdType offset_line_;
    vtkIdType probe_input_;
};
