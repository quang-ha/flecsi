#if defined(ENABLE_VTK)

#ifndef _UNSTRUCTURED_GRID_H_
#define _UNSTRUCTURED_GRID_H_

#include <string>
#include <iostream>

#include <mpi.h>

#include <vtkSmartPointer.h>
#include <vtkDoubleArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLPUnstructuredGridWriter.h>
#include <vtkSOADataArrayTemplate.h>
#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkPoints.h>
#include <vtkPointData.h>

#include <vtkMPIController.h>
#include <vtkXMLPUnstructuredGridWriter.h>
#include <vtkIntArray.h>
#include <vtkDataArray.h>
#include <vtkGenericDataArray.h>
#include <limits>

namespace vtkOutput
{

template<typename T>

struct point
{
	float x, y, z;

	point(){ x=y=z=0; }
	point(float _x, float _y, float _z){ x=_x; y=_y; z=_z; }


	bool equal(float _x, float _y, float _z)
	{
		bool match = std::fabs(x - _x) < std::numeric_limits<float>::epsilon();
		match = match && std::fabs(y - _y) < std::numeric_limits<float>::epsilon();
		match = match && std::fabs(z - _z) < std::numeric_limits<float>::epsilon();
		return match;
	}
};




class UnstructuredGrid
{
	vtkSmartPointer<vtkXMLPUnstructuredGridWriter> writer;
	vtkSmartPointer<vtkUnstructuredGrid> uGrid;

	vtkSmartPointer<vtkPoints> pnts;
  	vtkSmartPointer<vtkCellArray> cells;
  	vtkIdType idx;

  public:
	UnstructuredGrid();
	~UnstructuredGrid(){};
	

	vtkSmartPointer<vtkUnstructuredGrid> getUGrid(){ return uGrid; }

	// Topology
	template <typename T> void addVertex(T *pointData);
	template <typename T> void addCell(vtkSmartPointer<T> cellObj);
	template <typename T> void addPoint(T *pointData);
	void pushTopologyToGrid(int cellType);
	void setTopology(vtkSmartPointer<vtkPoints> _pnts, vtkSmartPointer<vtkCellArray> _cells, int cellType);

	// Data
	template <typename T> void addFieldScalar(std::string fieldName, T *data);

	template <typename T> void addScalarData(std::string scalarName, int numPoints, T *data, int kind=0);
	template <typename T> void addVectorData(std::string scalarName, int numPoints, int numComponents, T *data, int kind=0);
	

	// Writing
	void writeParts(int numPieces, int startPiece, int SetEndPiece, std::string fileName);
	void write(std::string fileName, int parallel=0);
};



inline UnstructuredGrid::UnstructuredGrid()
{
	writer = vtkSmartPointer<vtkXMLPUnstructuredGridWriter>::New();
	uGrid = vtkSmartPointer<vtkUnstructuredGrid>::New();

	pnts = vtkSmartPointer<vtkPoints>::New();
  	cells = vtkSmartPointer<vtkCellArray>::New();

  	idx = 0;
}




//
// Topology
template <typename T> 
inline void UnstructuredGrid::addVertex(T *pointData)
{
	pnts->InsertNextPoint(pointData);
}


template <typename T> 
inline void UnstructuredGrid::addCell(vtkSmartPointer<T> cellObj)
{
	cells->InsertNextCell(cellObj);
}



template <typename T> 
inline void UnstructuredGrid::addPoint(T *pointData)
{
	pnts->InsertPoint(idx, pointData);
    cells->InsertNextCell(VTK_VERTEX, &idx);
    idx++;
}

inline void UnstructuredGrid::pushTopologyToGrid(int cellType)
{
	uGrid->SetPoints(pnts);
  	uGrid->SetCells(cellType, cells);
}


inline void UnstructuredGrid::setTopology(vtkSmartPointer<vtkPoints> _pnts, vtkSmartPointer<vtkCellArray> _cells, int cellType)
{
	uGrid->SetPoints(_pnts);
  	uGrid->SetCells(cellType, _cells);
}


// Attributes
template <typename T>
inline void UnstructuredGrid::addFieldScalar(std::string fieldName, T *data)
{
  	vtkAOSDataArrayTemplate<T>* temp = vtkAOSDataArrayTemplate<T>::New();

  	temp->SetNumberOfTuples(1);
  	temp->SetNumberOfComponents(1);
  	temp->SetName(fieldName.c_str());
  	temp->SetArray(data, 1, false, true);

  	uGrid->GetFieldData()->AddArray(temp);
}


//
// Data
template <typename T>
inline void UnstructuredGrid::addScalarData(std::string varName, int numPoints, T *data, int kind)
{
	vtkSOADataArrayTemplate<T>* temp = vtkSOADataArrayTemplate<T>::New();

  	temp->SetNumberOfTuples(numPoints);
  	temp->SetNumberOfComponents(1);
  	temp->SetName(varName.c_str());
  	temp->SetArray(0, data, numPoints, false, true);

  	if (kind == 0)	// point
  		uGrid->GetPointData()->AddArray(temp);
  	else 			// cell
  		uGrid->GetCellData()->AddArray(temp);

  	temp->Delete();
}


template <typename T>
inline void UnstructuredGrid::addVectorData(std::string varName, int numPoints, int numComponents, T *data, int kind)
{
	vtkAOSDataArrayTemplate<T>* temp = vtkAOSDataArrayTemplate<T>::New();

  	temp->SetNumberOfTuples(numPoints);
  	temp->SetNumberOfComponents(numComponents);
  	temp->SetName(varName.c_str());
  	temp->SetArray(data, numPoints*numComponents, false, true);

  	if (kind == 0)	// point
  		uGrid->GetPointData()->AddArray(temp);
  	else 			// cell
  		uGrid->GetCellData()->AddArray(temp);


  	temp->Delete();
}



//
// Writing
inline void UnstructuredGrid::writeParts(int numPieces, int startPiece, int endPiece, std::string fileName)
{
	writer->SetNumberOfPieces(numPieces);
	writer->SetStartPiece(startPiece);
	writer->SetEndPiece(endPiece);

	write(fileName, 1);
}


inline void UnstructuredGrid::write(std::string fileName, int parallel)
{
	std::string outputFilename;

	if (parallel == 1)
		outputFilename = fileName + ".pvtu";
	else
		outputFilename = fileName + ".vtu";

    writer->SetDataModeToBinary();
    writer->SetCompressor(nullptr);
	writer->SetFileName(outputFilename.c_str());


	#if VTK_MAJOR_VERSION <= 5
        writer->SetInput(uGrid);
    #else
        writer->SetInputData(uGrid);
    #endif

	writer->Write();
}

} // vtkOutput

#endif	//_UNSTRUCTURED_GRID_H_
#endif	//ENABLE_VTK

