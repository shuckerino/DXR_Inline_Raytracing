/// Cogra --- Coburg Graphics Framework
/// (C) 2017-2022 by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#pragma once
#include <gimslib/types.hpp>
#include <memory>
#include <string>
#include <vector>
//! Namespace for everything that is Computer Graphics related.
namespace gims
{
//! \brief Binary file for triangle meshes.
//!
//! Topology is encoded as an indexed face set. Three indices to vertices make a triangle. Each vertex has a position
//! consisting of three floating point numbers. Additionally, each vertex may have an arbitrary number of attributes
//! of arbitrary type.
//! Moreover, values that are constant for the entire triangle mesh may be stored. Similar to attributes, an
//! arbitrary number of constants of arbitrary type is supported.
class CograBinaryMeshFile
{
  //! Maximum number of characters used for attribute and constant names
  enum
  {
    N_CHARS = 256
  };

public:
  //! Type for numbers and sizes.
  typedef ui32 SizeType;

  //! Type of the index used for vertex indices.
  typedef ui32 IndexType;

  //! Floating point used for vertex positions.
  typedef f32 FloatType;

  //! \brief Default constructor.
  CograBinaryMeshFile() = default;

  //! Copy construction.
  CograBinaryMeshFile(const CograBinaryMeshFile& other);

  //! Move construction.
  CograBinaryMeshFile(CograBinaryMeshFile&& other) noexcept;

  //! \brief Loads a file.
  //! \param[in]  fileName Path to QMB file that should be opened.
  explicit CograBinaryMeshFile(const std::string& fileName);

  //! \brief Default destructor.
  virtual ~CograBinaryMeshFile();

  void swap(CograBinaryMeshFile& other);

  //! Assignment operator.
  CograBinaryMeshFile& operator=(CograBinaryMeshFile other);

  //! Move operator.
  CograBinaryMeshFile& operator=(CograBinaryMeshFile&& other) noexcept;

  //! \brief Loads a file.
  //!
  //! \param[in]  fileName Path to file name
  void load(const std::string& fileName);

  //! \brief Saves a file.
  //!
  //! \param[in]  fileName Path to file name
  void save(const std::string& fileName);

  //! \brief Returns the number of vertices.
  SizeType getNumVertices() const;

  //! \brief Returns the number of triangles.
  SizeType getNumTriangles() const;

  //! \brief Returns a pointer to the vertex positions.
  const FloatType* getPositionsPtr() const;

  //! \brief Returns a pointer to the vertex positions.
  FloatType* getPositionsPtr();

  //! \brief Returns a pointer to the triangle index buffer.
  const IndexType* getTriangleIndices() const;

  //! \brief Returns a pointer to the triangle index buffer.
  IndexType* getTriangleIndices();

  //! \brief Sets the pointer to the vertices.
  //!
  //! \param[in]  positions Pointer to the vertex positions.
  //! \param[in]  nVertices Number of vertices.
  void setPositions(const FloatType* positions, SizeType nVertices);

  //! \brief Sets the triangle index buffers.
  //!
  //! \param[in]  triIdx Pointer to the triangle index buffer.
  //! \param[in]  nTriangles Number of triangles.
  void setTriangleIndices(const IndexType* triIdx, SizeType nTriangles);

  //! \brief Adds an attribute. The number of attributes should match the number of vertices.
  //!
  //! \param[in]  attribute Pointer to the attribute array.
  //! \param[in]  nComponents Number of components of each attribute element. For example, a normal vector attribute has
  //! 3 components. So you would place a 3 here. \param[in]  componentSize Number of bytes each component has. For
  //! example, a normal vector consists of floats, i.e. sizeof(f32) = 4. So you would place a four here. \param[in]
  //! attributeName Name of the attribute. Names having more than N_CHARS characters are chopped. \return Number of
  //! attribute arrays.
  SizeType addAttribute(const void* attribute, SizeType nComponents, SizeType componentSize,
                        const std::string& attributeName);

  //! \brief Adds a constant.
  //!
  //! \param[in]  constant Pointer to the constant.
  //! \param[in]  nComponents Number of components the constant. For example, a light direction vector has 3 components.
  //! So you would place a 3 here. \param[in]  componentSize Number of bytes each component has. For example, a light
  //! direction vector consists of floats, i.e. sizeof(f32) = 4. So you would place a four here. \param[in]  name Name
  //! of the constant. Names having more than N_CHARS characters are chopped. \return Number of constants.
  SizeType addConstant(const void* constant, SizeType nComponents, SizeType componentSize, const std::string& name);

  //! \brief Returns a void* to an attribute array.
  //!
  //! \param  attributeIdx Index of the attribute.
  void* getAttributePtr(SizeType attributeIdx) const;

  //! \brief Returns a void* to the constant.
  //!
  //! \param[in]  constantIdx Index of the constant.
  void* getConstant(SizeType constantIdx) const;

  //! \brief Returns an integer constant based on name.
  //! \param[in]  name Returns an integer constant.
  //! \param[in]  ok True of the constant was found, false otherwise.
  //! \return Return the integer constant.
  int getIntegerConstant(const char* name, bool* ok = nullptr) const;

  //! \brief Replaces the attribute which index attributeIdx by a new array.
  //!
  //! \param[in]  attributeIdx
  //! \param[in]  attribute Pointer to the new array.
  //! \return 0 on error, pointer to the array on success.
  void* replaceAttribute(SizeType attributeIdx, const void* attribute);

  //! \brief Returns the size of one component of an attribute.
  //!
  //! For a normal vector that would be 4, as a normal vector consists of floats, and sizeof(f32)=4.
  //!
  //! \param[in]  attributeIdx Index of the attribute.
  //! \return The size of one component of an attribute.
  SizeType getAttributeComponentSize(SizeType attributeIdx) const;

  //! \brief Returns the number of components an attribute element has.
  //!
  //! For a normal vector 3 would be returned, for a 2d texture coordinate 2 would be returned.
  //!
  //! \param[in]  attributeIdx Index of the attribute.
  //! \return The number of components an attribute element has.
  SizeType getAttributeComponents(SizeType attributeIdx) const;

  //! \brief Returns the size in bytes of an attribute.
  //!
  //! This is product of attributeComponentSize(attributeIdx) * attributeComponents(attributeIdx).
  //!
  //! \param  attributeIdx Index of the attribute.
  //! \return Returns the size in bytes of an attribute.
  SizeType getAttributeElementSize(SizeType attributeIdx) const;

  //! \brief Returns the attribute name.
  //!
  //! \param[in]  attributeIdx Index of the attribute.
  //! \return The name of the attribute.
  const char* getAttributeName(SizeType attributeIdx) const;

  //! \brief Number of attributes.
  SizeType getNumAttributes() const;

  //! \brief Total size in bytes of all attributes.
  SizeType getTotalAttributeSize() const;

  //! \brief Returns the vertex position and its attributes of the vertex vIdx into result.
  //!
  //! \param  result Must have at least 12 bytes (for the vertex position) plus attributeSize() for the attributes.
  //! \param  vIdx Index of the vertex
  void getAllVertexAttributes(void* result, SizeType vIdx) const;

  //! \brief Returns the size of one component of a constant.
  //!
  //! For a light direction vector that would be 4, as a light direction vector consists of floats, and sizeof(f32)=4.
  //!
  //! \param  constantIdx Index of the constant.
  //! return
  SizeType getConstantComponentSize(SizeType constantIdx) const;

  //! \brief Returns the number of components a constant element has.

  //! For a light direction vector 3 would be returned, as a light direction has three components.
  //!
  //! \param  constantIdx Index of the attribute.
  SizeType getConstantComponents(SizeType constantIdx) const;

  //! \brief Size in bytes of a constant.
  //!
  //! This is product of constantComponentSize(constantIdx) * constantComponents(constantIdx).
  //!
  //! \param constantIdx  Index of the constant.
  //! \return Size in bytes of a constant.
  SizeType getConstantElementSize(SizeType constantIdx) const;

  //! \brief Returns the constant name.
  //!
  //! \param[in]  constantIdx Index of the constantIdx.
  //! \return Name of a constant.
  const char* getConstantName(SizeType constantIdx) const;

  //! \brief Number of constants.
  SizeType getNumConstants() const;

  //! \brief Reads the header.
  //! \param[in,out]  inFile Reference to an opened file.
  void readHeader(std::ifstream& inFile);

  //! \brief Writes the header.
  //! \param[in,out]  outFile Reference to an opened file.
  void writeHeader(std::ofstream& outFile);

  //! \brief Deletes all attributes.
  void freeAttributes();

  //! \brief Deletes all constants.
  void freeConstants();

  //! \brief Adds another QMBinFile to this QMBinFile.
  //!
  //! \param  src Bin file that should be appended to this file.
  //! \return True on success, false otherwise.
  bool add(const CograBinaryMeshFile& src);

  //! \brief Prints the information about constants to a stream.
  //! \param[in,out]  stream The stream the information should be written to.
  void printConstant(std::ostream& stream) const;

  //! \brief Prints the information about attributes to a stream.
  //! \param[in,out]  stream The stream the information should be written to.
  void printAttributes(std::ostream& stream) const;

  //! Overwrites the constants of this file with the constants of src.
  //! \param[in] src Source file from the constants should be read from.
  void overwriteConstants(const CograBinaryMeshFile& src);

  //! \brief Returns the index of a constant.
  //! \param[in]  name Name of the constant.
  int getConstantIdx(const char* name) const;

  //! \brief Returns the index of a constant.
  //!
  //! \param[in]  components Number of components.
  //! \param[in]  componentSize Size of one component.
  //! \param[in]  name Name of the constant.
  //! \return -1 if constant does not exist, otherwise the constant index.
  int getConstantIdx(SizeType components, SizeType componentSize, const char* name) const;

private:
  //! Vertex positions.
  std::vector<FloatType> m_positions;

  //! Indexed face set of triangles.
  std::vector<IndexType> m_triangles;

  //! All the attributes
  std::vector<ui8*> m_attributes;

  //! Stores the number of components an attribute element posses (e.g., a normal has three components).
  std::vector<SizeType> m_attributeComponents;

  //! Holds the size in bytes of each component of an attribute element, e.g., a normal component has size 4, if it
  //! is a f32.
  std::vector<SizeType> m_attributeComponentSize;

  //! The attribute names.
  std::vector<char*> m_attributeNames;

  //! Constants used for example for material properties.
  std::vector<ui8*> m_constants;

  //! Stores the number of components a constant posses (e.g., a light direction vector has three components).
  std::vector<SizeType> m_constantComponents;

  //! Holds the size in bytes of each component of a constant, e.g., a light direction vector component has size 4,
  //! if it is a f32.
  std::vector<SizeType> m_constantComponentSize;

  //! The constant names.
  std::vector<char*> m_constantNames;
};
} // namespace gims
