/// Cogra --- Coburg Graphics Framework
/// (C) 2017-2022 by Quirin Meyer
/// quirin.meyer@hs-coburg.de
#include <algorithm>
#include <cstring>
#include <fstream>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <istream>
#include <ostream>

namespace gims
{

CograBinaryMeshFile::CograBinaryMeshFile(const CograBinaryMeshFile& other)
{
  m_positions              = other.m_positions;
  m_triangles              = other.m_triangles;
  m_attributeComponents    = other.m_attributeComponents;
  m_attributeComponentSize = other.m_attributeComponentSize;
  m_constantComponents     = other.m_constantComponents;
  m_constantComponentSize  = other.m_constantComponentSize;

  for (size_t i = 0; i < other.m_attributes.size(); i++)
  {
    const auto nBytes   = m_attributeComponents[i] * m_attributeComponentSize[i] * getNumVertices();
    m_attributes[i]     = new ui8[nBytes];
    m_attributeNames[i] = new char[N_CHARS];
    std::copy(other.m_attributes[i], other.m_attributes[i] + nBytes, m_attributes[i]);
    std::copy(other.m_attributeNames[i], other.m_attributeNames[i] + sizeof(char) * N_CHARS, m_attributeNames[i]);
  }

  for (SizeType i = 0; i < getNumConstants(); i++)
  {
    const auto nBytes  = m_constantComponents[i] * m_constantComponentSize[i];
    m_constants[i]     = new ui8[nBytes];
    m_constantNames[i] = new char[N_CHARS];
    std::copy(other.m_constants[i], other.m_constants[i] + nBytes, m_constants[i]);
    std::copy(other.m_constantNames[i], other.m_constantNames[i] + sizeof(char) * N_CHARS, m_constantNames[i]);
  }
}

CograBinaryMeshFile::CograBinaryMeshFile(CograBinaryMeshFile&& other) noexcept
    : m_positions(std::exchange(other.m_positions, {}))
    , m_triangles(std::exchange(other.m_triangles, {}))
    , m_attributes(std::exchange(other.m_attributes, {}))
    , m_attributeComponents(std::exchange(other.m_attributeComponents, {}))
    , m_attributeComponentSize(std::exchange(other.m_attributeComponentSize, {}))
    , m_attributeNames(std::exchange(m_attributeNames, {}))
    , m_constants(std::exchange(m_constants, {}))
    , m_constantComponents(std::exchange(m_constantComponents, {}))
    , m_constantComponentSize(std::exchange(m_constantComponentSize, {}))
    , m_constantNames(std::exchange(m_constantNames, {}))
{
}

CograBinaryMeshFile::CograBinaryMeshFile(const std::string& fileName)
{
  load(fileName);
}

CograBinaryMeshFile::~CograBinaryMeshFile()
{
  freeAttributes();
  freeConstants();
}

CograBinaryMeshFile& CograBinaryMeshFile::operator=(CograBinaryMeshFile other)
{
  other.swap(*this);
  return *this;
}

CograBinaryMeshFile& CograBinaryMeshFile::operator=(CograBinaryMeshFile&& other) noexcept
{
  other.swap(*this);
  return *this;
}

void CograBinaryMeshFile::swap(CograBinaryMeshFile& other)
{
  m_positions.swap(other.m_positions);
  m_triangles.swap(other.m_triangles);
  m_attributes.swap(other.m_attributes);
  m_attributeComponents.swap(other.m_attributeComponents);
  m_attributeComponentSize.swap(other.m_attributeComponentSize);
  m_attributeNames.swap(other.m_attributeNames);
  m_constants.swap(other.m_constants);
  m_constantComponents.swap(other.m_constantComponents);
  m_constantComponentSize.swap(other.m_constantComponentSize);
  m_constantNames.swap(other.m_constantNames);
}

void CograBinaryMeshFile::load(const std::string& fileName)
{
  std::ifstream inFile;

  inFile.open(fileName, std::ios::in | std::ios::binary);
  if (!inFile.is_open())
  {
    throw std::runtime_error("Error opening file" + fileName + ".");
  }
  inFile.exceptions(std::ifstream::eofbit | std::ifstream::failbit | std::ifstream::badbit);

  readHeader(inFile);
  // read vertices
  inFile.read((char*)&m_positions[0], sizeof(FloatType) * 3 * getNumVertices());
  inFile.read((char*)&m_triangles[0], sizeof(IndexType) * 3 * getNumTriangles());

  for (SizeType i = 0; i < getNumAttributes(); i++)
  {
    inFile.read((char*)m_attributes[i], getAttributeElementSize(i) * getNumVertices());
  }

  for (SizeType i = 0; i < getNumConstants(); i++)
  {
    inFile.read((char*)m_constants[i], getConstantElementSize(i));
  }
}

void CograBinaryMeshFile::save(const std::string& fileName)
{
  std::ofstream outFile;
  outFile.open(fileName, std::ios::out | std::ios::binary);
  writeHeader(outFile);
  outFile.write((const char*)&m_positions[0], sizeof(FloatType) * 3 * getNumVertices());
  outFile.write((const char*)&m_triangles[0], sizeof(IndexType) * 3 * getNumTriangles());
  for (SizeType i = 0; i < getNumAttributes(); i++)
  {
    SizeType size = getAttributeElementSize(i) * getNumVertices();
    outFile.write((const char*)m_attributes[i], size);
  }

  for (SizeType i = 0; i < getNumConstants(); i++)
  {
    SizeType size = getConstantElementSize(i);
    outFile.write((const char*)m_constants[i], size);
  }
  outFile.close();
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getNumVertices() const
{
  return static_cast<ui32>(m_positions.size() / 3);
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getNumTriangles() const
{
  return static_cast<ui32>(m_triangles.size() / 3);
}

const CograBinaryMeshFile::FloatType* CograBinaryMeshFile::getPositionsPtr() const
{
  return m_positions.data();
}

CograBinaryMeshFile::FloatType* CograBinaryMeshFile::getPositionsPtr()
{
  return m_positions.data();
}

const CograBinaryMeshFile::IndexType* CograBinaryMeshFile::getTriangleIndices() const
{
  return &m_triangles[0];
}

CograBinaryMeshFile::IndexType* CograBinaryMeshFile::getTriangleIndices()
{
  return &m_triangles[0];
}

void CograBinaryMeshFile::setPositions(const FloatType* vertices, const SizeType nVertices)
{
  m_positions.resize(nVertices * 3);
  for (SizeType i = 0; i < nVertices * 3; i++)
  {
    m_positions[i] = vertices[i];
  }
}

void CograBinaryMeshFile::setTriangleIndices(const IndexType* triIdx, const SizeType nTriangles)
{
  m_triangles.resize(nTriangles * 3);
  for (SizeType i = 0; i < nTriangles * 3; i++)
  {
    m_triangles[i] = triIdx[i];
  }
}

void CograBinaryMeshFile::readHeader(std::ifstream& inFile)
{
  SizeType nV;
  SizeType nT;
  SizeType nA;
  SizeType nC;
  freeAttributes();
  inFile.read((char*)&nV, sizeof(SizeType));
  m_positions.resize(nV * 3);
  inFile.read((char*)&nT, sizeof(SizeType));
  m_triangles.resize(nT * 3);
  inFile.read((char*)&nA, sizeof(SizeType));

  m_attributeComponents.resize(nA);
  m_attributeComponentSize.resize(nA);
  m_attributeNames.resize(nA);
  m_attributes.resize(nA);

  if (nA != 0)
  {
    inFile.read((char*)&m_attributeComponents[0], nA * sizeof(SizeType));
    inFile.read((char*)&m_attributeComponentSize[0], nA * sizeof(SizeType));

    for (SizeType i = 0; i < getNumAttributes(); i++)
    {
      m_attributes[i]     = new ui8[m_attributeComponents[i] * m_attributeComponentSize[i] * getNumVertices()];
      m_attributeNames[i] = new char[N_CHARS];
      std::memset(m_attributeNames[i], '\0', N_CHARS);
      inFile.read((char*)(m_attributeNames[i]), sizeof(char) * N_CHARS);
    }
  }

  inFile.read((char*)&nC, sizeof(SizeType));
  m_constantComponents.resize(nC);
  m_constantComponentSize.resize(nC);
  m_constantNames.resize(nC);
  m_constants.resize(nC);

  if (nC != 0)
  {
    inFile.read((char*)&m_constantComponents[0], nC * sizeof(SizeType));
    inFile.read((char*)&m_constantComponentSize[0], nC * sizeof(SizeType));

    for (SizeType i = 0; i < getNumConstants(); i++)
    {
      m_constants[i]     = (new ui8[m_constantComponents[i] * m_constantComponentSize[i]]);
      m_constantNames[i] = new char[N_CHARS];
      inFile.read((char*)m_constantNames[i], sizeof(ui8) * N_CHARS);
    }
  }
}

void CograBinaryMeshFile::writeHeader(std::ofstream& outFile)
{
  SizeType nV = getNumVertices();
  SizeType nT = getNumTriangles();
  SizeType nA = getNumAttributes();
  SizeType nC = getNumConstants();
  outFile.write((const char*)&nV, sizeof(SizeType));
  outFile.write((const char*)&nT, sizeof(SizeType));
  outFile.write((const char*)&nA, sizeof(SizeType));
  if (nA != 0)
  {
    outFile.write((const char*)&m_attributeComponents[0], nA * sizeof(SizeType));
    outFile.write((const char*)&m_attributeComponentSize[0], nA * sizeof(SizeType));
    for (SizeType i = 0; i < nA; i++)
    {
      outFile.write((const char*)(m_attributeNames[i]), sizeof(char) * N_CHARS);
    }
  }
  outFile.write((const char*)&nC, sizeof(SizeType));
  if (nC != 0)
  {
    outFile.write((const char*)&m_constantComponents[0], nC * sizeof(SizeType));
    outFile.write((const char*)&m_constantComponentSize[0], nC * sizeof(SizeType));
    for (SizeType i = 0; i < nC; i++)
    {
      outFile.write((const char*)(m_constantNames[i]), sizeof(char) * N_CHARS);
    }
  }
}

bool CograBinaryMeshFile::add(const CograBinaryMeshFile& src)
{
  // checks
  if (this->getNumAttributes() != src.getNumAttributes())
  {
    return false;
  }

  const SizeType nAttributes = src.getNumAttributes();

  for (SizeType i = 0; i < nAttributes; i++)
  {
    if (this->getAttributeComponentSize(i) != src.getAttributeComponentSize(i))
    {
      return false;
    }
    if (this->getAttributeComponents(i) != src.getAttributeComponents(i))
    {
      return false;
    }
  }

  // merge

  auto* vertices = new FloatType[(this->getNumVertices() + src.getNumVertices()) * 3];
  memcpy((void*)&vertices[0], (void*)this->getPositionsPtr(), this->getNumVertices() * 3 * sizeof(FloatType));
  memcpy((void*)&vertices[this->getNumVertices() * 3], (void*)src.getPositionsPtr(),
         src.getNumVertices() * 3 * sizeof(FloatType));

  auto* idxBuffer = new IndexType[(this->getNumTriangles() + src.getNumTriangles()) * 3];
  memcpy((void*)&idxBuffer[0], (void*)this->getTriangleIndices(), this->getNumTriangles() * 3 * sizeof(IndexType));
  // translate index buffer
  for (SizeType i = 0; i < src.getNumTriangles() * 3; i++)
  {
    idxBuffer[this->getNumTriangles() * 3 + i] = src.getTriangleIndices()[i] + getNumVertices();
  }

  std::vector<ui8*> attributes(nAttributes);
  for (SizeType i = 0; i < nAttributes; i++)
  {
    SizeType attribSize = src.getAttributeComponents(i) * src.getAttributeComponentSize(i);
    auto     attr       = new ui8[(this->getNumVertices() + src.getNumVertices()) * attribSize];
    memcpy((void*)&attr[0], this->getAttributePtr(i), this->getNumVertices() * attribSize);
    memcpy((void*)&attr[this->getNumVertices() * attribSize], src.getAttributePtr(i),
           src.getNumVertices() * attribSize);
    attributes[i] = attr;
  }

  this->setPositions(vertices, this->getNumVertices() + src.getNumVertices());
  this->setTriangleIndices(idxBuffer, this->getNumTriangles() + src.getNumTriangles());

  for (SizeType i = 0; i < nAttributes; i++)
  {
    this->replaceAttribute(i, attributes[i]);
  }

  // tidy up
  for (SizeType i = 0; i < nAttributes; i++)
  {
    delete[] attributes[i];
  }
  return true;
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getNumAttributes() const
{
  return static_cast<ui32>(m_attributeComponents.size());
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::addAttribute(const void* attribute, const SizeType nComponents,
                                                                const SizeType     componentSize,
                                                                const std::string& attributeName)
{
  SizeType size = getNumVertices() * nComponents * componentSize;
  auto*    p    = new ui8[size];

  memcpy((void*)p, attribute, size);
  m_attributes.push_back(p);
  m_attributeComponentSize.push_back(componentSize);
  m_attributeComponents.push_back(nComponents);

  auto* a = new char[N_CHARS];
  memset((void*)a, '\0', N_CHARS);
  SizeType l = std::min<SizeType>(static_cast<ui32>(N_CHARS), static_cast<ui32>(attributeName.length()));
  for (SizeType i = 0; i < l; i++)
  {
    a[i] = attributeName[i];
  }

  m_attributeNames.push_back(a);
  return static_cast<ui32>(m_attributes.size());
}

void* CograBinaryMeshFile::getAttributePtr(SizeType attributeIdx) const
{
  return m_attributes[attributeIdx];
}

void* CograBinaryMeshFile::replaceAttribute(SizeType attributeIdx, const void* attribute)
{
  if (attributeIdx >= m_attributes.size())
  {
    return nullptr;
  }
  SizeType size = getNumVertices() * m_attributeComponentSize[attributeIdx] * m_attributeComponents[attributeIdx];
  auto     p    = new ui8[size];
  memcpy((void*)p, attribute, size);
  delete[] m_attributes[attributeIdx];
  m_attributes[attributeIdx] = p;
  return p;
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getAttributeComponentSize(SizeType attributeIdx) const
{
  return m_attributeComponentSize[attributeIdx];
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getAttributeComponents(SizeType attributeIdx) const
{
  return m_attributeComponents[attributeIdx];
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getAttributeElementSize(SizeType attributeIdx) const
{
  return m_attributeComponentSize[attributeIdx] * m_attributeComponents[attributeIdx];
}

const char* CograBinaryMeshFile::getAttributeName(SizeType attributeIdx) const
{
  return m_attributeNames[attributeIdx];
}

void CograBinaryMeshFile::freeAttributes()
{
  for (SizeType i = 0; i < getNumAttributes(); i++)
  {
    if (m_attributes[i])
    {
      delete[] m_attributes[i];
      m_attributes[i] = nullptr;
    }
    m_attributeComponents[i]    = 0;
    m_attributeComponentSize[i] = 0;
    if (m_attributeNames[i])
    {
      delete[] m_attributeNames[i];
      m_attributeNames[i] = nullptr;
    }
  }
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getConstantComponentSize(SizeType constantIdx) const
{
  return m_constantComponentSize[constantIdx];
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getConstantComponents(SizeType constantIdx) const
{
  return m_constantComponents[constantIdx];
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getConstantElementSize(SizeType constantIdx) const
{
  return getConstantComponentSize(constantIdx) * getConstantComponents(constantIdx);
}

const char* CograBinaryMeshFile::getConstantName(SizeType constantIdx) const
{
  return m_constantNames[constantIdx];
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getNumConstants() const
{
  return static_cast<ui32>(m_constantComponentSize.size());
}

void CograBinaryMeshFile::freeConstants()
{
  for (SizeType i = 0; i < getNumConstants(); i++)
  {
    delete[] m_constants[i];
    m_constants[i]             = nullptr;
    m_constantComponents[i]    = 0;
    m_constantComponentSize[i] = 0;
    delete[] m_constantNames[i];
    m_constantNames[i] = nullptr;
  }
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::addConstant(const void* constant, const SizeType nComponents,
                                                               const SizeType     componentSize,
                                                               const std::string& constantName)
{
  SizeType size = nComponents * componentSize;
  auto     p    = new ui8[size];

  memcpy((void*)p, constant, size);
  m_constants.push_back(p);
  m_constantComponentSize.push_back(componentSize);
  m_constantComponents.push_back(nComponents);

  auto* a = new char[N_CHARS];
  memset((void*)a, '\0', N_CHARS);
  SizeType l = std::min<SizeType>((SizeType)N_CHARS, (SizeType)constantName.length());
  for (SizeType i = 0; i < l; i++)
  {
    a[i] = constantName[i];
  }

  m_constantNames.push_back(a);
  return (SizeType)m_constants.size();
}

void* CograBinaryMeshFile::getConstant(SizeType constantIdx) const
{
  return m_constants[constantIdx];
}

void CograBinaryMeshFile::overwriteConstants(const CograBinaryMeshFile& src)
{
  freeConstants();
  for (SizeType t = 0; t < src.getNumConstants(); t++)
  {
    addConstant(src.getConstant(t), src.getConstantComponents(t), src.getConstantComponentSize(t),
                src.getConstantName(t));
  }
}

int CograBinaryMeshFile::getConstantIdx(const char* name) const
{
  for (ui32 i = 0; i < getNumConstants(); i++)
    if (strcmp(name, m_constantNames[i]) == 0)
    {
      return i;
    }
  return -1;
}

int CograBinaryMeshFile::getConstantIdx(const SizeType components, const SizeType componentSize, const char* name) const
{
  // find constant name
  int i = getConstantIdx(name);
  // if constant name does not exists, exit.
  if (i == -1)
  {
    return -1;
  }
  // otherwise check if the number of components and the size of each component match.
  if ((getConstantComponents(static_cast<SizeType>(i)) == components) &&
      (getConstantComponentSize(static_cast<SizeType>(i)) == componentSize))
  {
    return i;
  }
  else
  {
    return -1;
  }
}

#define DEREF(TYPE, PTR) (*((TYPE*)(PTR)))

int CograBinaryMeshFile::getIntegerConstant(const char* name, bool* ok /*= 0*/) const
{
  int idx = getConstantIdx(1, sizeof(int), name);
  if (idx != -1)
  {
    if (ok)
    {
      *ok = true;
    }
    return DEREF(int, getConstant(static_cast<SizeType>(idx)));
  }
  else
  {
    if (ok)
    {
      *ok = false;
    }
    return 0;
  }
}

void CograBinaryMeshFile::getAllVertexAttributes(void* const result, const SizeType vIdx) const
{
  // Add the vertices
  ((f32* const)result)[vIdx + 0] = m_positions[vIdx + 0];
  ((f32* const)result)[vIdx + 1] = m_positions[vIdx + 1];
  ((f32* const)result)[vIdx + 2] = m_positions[vIdx + 2];
  SizeType offset                = 3 * 4;
  // Add the attributes.
  for (SizeType aIdx = 0; aIdx < getNumAttributes(); aIdx++)
    for (SizeType cIdx = 0; cIdx < getAttributeElementSize(aIdx); cIdx++)
    {
      ((unsigned char* const)result)[offset++] =
          ((unsigned char const*)getAttributePtr(aIdx))[vIdx * getAttributeElementSize(aIdx) + cIdx];
    }
}

void CograBinaryMeshFile::printAttributes(std::ostream& stream) const
{
  stream << "N Attributes: " << getNumAttributes() << "\n";
  stream << "Index\tSize\tN Comp.\tName\n";
  for (SizeType t = 0; t < getNumAttributes(); t++)
  {
    stream << t

           << "\t" << getAttributeComponentSize(t) << "\t" << getAttributeComponents(t) << "\t" << getAttributeName(t)
           << "\n";
  }
  stream << "\n";
}

void CograBinaryMeshFile::printConstant(std::ostream& stream) const
{
  stream << "N Constants: " << getNumConstants() << "\n";
  stream << "Index\tSize\tN Comp.\tName\n";
  for (SizeType t = 0; t < getNumConstants(); t++)
  {
    stream << t

           << "\t" << getConstantComponentSize(t) << "\t" << getConstantComponents(t) << "\t" << getConstantName(t)
           << "\n";
  }
  stream << "\n";
}

CograBinaryMeshFile::SizeType CograBinaryMeshFile::getTotalAttributeSize() const
{
  SizeType result = 0;
  for (SizeType i = 0; i < getNumAttributes(); i++)
  {
    result += getAttributeElementSize(i);
  }
  return result;
}

#undef DEREF
} // namespace gims
