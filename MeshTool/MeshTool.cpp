#include <iostream>
#include <map>
#include <vector>
#include <fstream>
#include <afx.h>

#define GROUP_ID "TClient"
#define MESHTYPE_MESH   ((BYTE) 0x01)
#define MESHTYPE_WMESH  ((BYTE) 0x02)

bool LoadMeshSeparately = true;
bool SortIntoFolders = true;

typedef std::map <DWORD, LPVOID> MAPRES, *LPMAPRES;
typedef std::vector <CString> VECTORSTRING, *LPVECTORSTRING;

template <typename T>
using VECTOR = std::vector<T>;

typedef struct tagWMESHVERTEX				WMESHVERTEX, * LPWMESHVERTEX;
typedef struct tagMESHVERTEX				MESHVERTEX, * LPMESHVERTEX;
typedef struct tagD3DXVECTOR3				D3DXVECTOR3, * LPD3DXVECTOR3;
typedef struct tagD3DXMATRIX				D3DXMATRIX, * LPD3DXMATRIX;

struct tagWMESHVERTEX
{
    FLOAT m_fPosX;
    FLOAT m_fPosY;
    FLOAT m_fPosZ;

    FLOAT m_fWeight[3];
    DWORD m_dwMatIndex;

    FLOAT m_fNormalX;
    FLOAT m_fNormalY;
    FLOAT m_fNormalZ;

    FLOAT m_fU;
    FLOAT m_fV;
};

struct tagMESHVERTEX
{
    FLOAT m_fPosX;
    FLOAT m_fPosY;
    FLOAT m_fPosZ;

    FLOAT m_fNormalX;
    FLOAT m_fNormalY;
    FLOAT m_fNormalZ;

    FLOAT m_fU1;
    FLOAT m_fV1;
    FLOAT m_fU2;
    FLOAT m_fV2;
};

struct tagD3DXVECTOR3
{
    FLOAT x;
    FLOAT y;
    FLOAT z;
};

struct tagD3DXMATRIX
{
    FLOAT _11, _12, _13, _14;
    FLOAT _21, _22, _23, _24;
    FLOAT _31, _32, _33, _34;
    FLOAT _41, _42, _43, _44;
};

struct MeshStruct
{
    BYTE m_bMESHType;
    DWORD m_dwMeshCount;
    DWORD m_dwNodeCount;
    BYTE m_bUseVB;
    DWORD m_dwLevel;
    D3DXVECTOR3 m_vCenterPoint;
    FLOAT m_fRadius;
    VECTOR<D3DXMATRIX> m_vBones;
    VECTOR<MESHVERTEX> m_vMESHVERTEX;
    VECTOR<WMESHVERTEX> m_vWMESHVERTEX;
    DWORD m_dwIndicesCount;
    WORD m_wMatrix;
    VECTOR<WORD> m_vIndices;
    VECTOR<FLOAT> m_vDist;
    CString strFile;
};

class MeshTool
{
public:
    MeshTool(CString strPath, CString strGroupID)
    {
        this->strPath = strPath;
        this->strGroupID = strGroupID;
    }
    ~MeshTool()
    {

    }

public:
    void create_directory(char* Path) 
    {
        char DirName[256];
        char* p = Path;
        char* q = DirName;

        while (*p)
        {
            if (('\\' == *p) || ('/' == *p))
            {
                if (':' != *(p - 1))
                {
                    CreateDirectory(DirName, NULL);
                }
            }
            *q++ = *p++;
            *q = '\0';
        }
        CreateDirectory(DirName, NULL);
    }

    bool exist_directory(const std::string& dirName_in)
    {
        DWORD ftyp = GetFileAttributesA(dirName_in.c_str());
        if (ftyp == INVALID_FILE_ATTRIBUTES)
            return false;
        if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
            return true;
        return false; 
    }

    CString ReadIndexString(CFile& file)
    {
        int nLength = 0;
        char bChar = 0x00;

        CString strRESULT;

        file.Read(&nLength, sizeof(int));
        for (int i = 0; i < nLength; i++)
        {
            file.Read(&bChar, sizeof(char));
            strRESULT += bChar;
        }
        
        return strRESULT;
    }

    void ReadIndex(CString strData)
    {
        int nTotal = 0;
        int nCount = 0;
        int nIndex = 0;

        CString strINDEX;
        strINDEX.Format(strPath, strGroupID);

        CFile file(strINDEX, CFile::modeRead | CFile::typeBinary);
        file.Read(&nCount, sizeof(int));
        file.Read(&nTotal, sizeof(int));

        LPMAPRES pTRES = new MAPRES[nCount];

        for (int i = 0; i < nCount; i++)
        {
            m_vMESHFILE.push_back(ReadIndexString(file));
            LoadMESH(DWORD(i), &pTRES[i], nIndex, nTotal, strData);
        }

        for (int i = 0; i < nTotal; i++)
        {
            DWORD dwFileID;
            DWORD dwPOS;
            DWORD dwID;

            file.Read(&dwID, sizeof(DWORD));
            file.Read(&dwFileID, sizeof(DWORD));
            file.Read(&dwPOS, sizeof(DWORD));

            MAPRES::iterator finder = pTRES[dwFileID].find(dwPOS);
            if (finder != pTRES[dwFileID].end())
            {
                m_mapMESH.insert(MAPRES::value_type(dwID, (*finder).second));
                pTRES[dwFileID].erase(finder);
            }
        }

        for (int i = 0; i < nCount; i++)
        {
            MAPRES::iterator it;
            for (it = pTRES[i].begin(); it != pTRES[i].end(); it++)
            {
                delete (*it).second;
            }
            pTRES[i].clear();
        }

        delete[] pTRES;
    }

    void LoadMESH(DWORD dwFileID, LPMAPRES pTRES, int& nIndex, int nTotal, CString strData)
    {
        if (SortIntoFolders && !LoadMeshSeparately)
        {
            ::MessageBox(NULL, "Can't sort into folders directly!", "Alert", MB_OK);
            ::exit(NULL);
        }

        CFile file(strData + m_vMESHFILE[dwFileID], CFile::modeRead | CFile::typeBinary);

        DWORD dwLENGTH = (DWORD)file.GetLength();
        DWORD dwPOS = (DWORD)file.GetPosition();

        while (dwPOS < dwLENGTH)
        {
            MeshStruct* pMESH = new MeshStruct();

            DWORD dwCount = 0;
            DWORD dwLevel = 0;

            file.Read(&pMESH->m_dwMeshCount, sizeof(DWORD));
            file.Read(&pMESH->m_dwNodeCount, sizeof(DWORD));
            file.Read(&pMESH->m_bUseVB, sizeof(BYTE));
            file.Read(&pMESH->m_dwLevel, sizeof(DWORD));
            file.Read(&pMESH->m_vCenterPoint, sizeof(D3DXVECTOR3));
            file.Read(&pMESH->m_fRadius, sizeof(FLOAT));
            pMESH->strFile = m_vMESHFILE[dwFileID];

            pMESH->m_bMESHType = pMESH->m_dwNodeCount > 0 ? MESHTYPE_WMESH : MESHTYPE_MESH;

            if (pMESH->m_dwNodeCount > 0)
            {
                D3DXMATRIX dwBone;
                for (DWORD i = 0; i < pMESH->m_dwNodeCount; i++)
                {
                    file.Read(&dwBone, sizeof(D3DXMATRIX));
                    pMESH->m_vBones.push_back(dwBone);
                }
            }

            CString strPath, strData;
            CFile save;
            if (!LoadMeshSeparately)
            {
                strPath.Format(".\\Data\\MESHES\\%d.obj", dwPOS);
                save.Open(strPath, CFile::modeCreate | CFile::modeWrite);
            }

            file.Read(&dwCount, sizeof(DWORD));
            if (dwCount > 0)
            {
                WMESHVERTEX wMESH;
                MESHVERTEX MESH;
                for (DWORD i = 0; i < dwCount; i++)
                {
                    if (pMESH->m_dwNodeCount > 0)
                    {
                        file.Read(&wMESH, sizeof(WMESHVERTEX));
                        if (!LoadMeshSeparately)
                        {
                            strData.Format("v %f %f %f\n", wMESH.m_fPosX, wMESH.m_fPosY, wMESH.m_fPosZ);
                            save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                            strData.Format("vt %f %f\n", wMESH.m_fU, wMESH.m_fV);
                            save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                            strData.Format("vn %f %f %f\n", wMESH.m_fNormalX, wMESH.m_fNormalY, wMESH.m_fNormalZ);
                            save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                        }
                        else
                        {
                            pMESH->m_vWMESHVERTEX.push_back(wMESH);
                        }
                    }
                    else
                    {
                        file.Read(&MESH, sizeof(MESHVERTEX));
                        if (!LoadMeshSeparately)
                        {
                            strData.Format("v %f %f %f\n", MESH.m_fPosX, MESH.m_fPosY, MESH.m_fPosZ);
                            save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                            strData.Format("vt %f %f\n", MESH.m_fU1, MESH.m_fV1);
                            save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                            strData.Format("vt1 %f %f\n", MESH.m_fU2, MESH.m_fV2);
                            save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                            strData.Format("vn %f %f %f\n", MESH.m_fNormalX, MESH.m_fNormalY, MESH.m_fNormalZ);
                            save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                        }
                        else
                        {
                            pMESH->m_vMESHVERTEX.push_back(MESH);
                        }
                    }
                }
            }

            for (SIZE_T i = 0; i < pMESH->m_dwMeshCount; i++)
            {
                for (SIZE_T j = 0; j < pMESH->m_dwLevel; j++)
                {
                    file.Read(&dwCount, sizeof(DWORD));
                    for (DWORD k = 0; k < dwCount; k++)
                    {
                        file.Read(&pMESH->m_dwIndicesCount, sizeof(DWORD));
                        file.Read(&pMESH->m_wMatrix, sizeof(WORD));
                        if (pMESH->m_dwIndicesCount > 0)
                        {
                            WORD wIndice;
                            for (DWORD l = 0; l < pMESH->m_dwIndicesCount; l++)
                            {
                                file.Read(&wIndice, sizeof(WORD));
                                pMESH->m_vIndices.push_back(wIndice);
                            }
                        }
                    }
                }
            }


            if (!LoadMeshSeparately)
            {
                for (SIZE_T i = 0; i < pMESH->m_vIndices.size(); i += 3)
                {
                    int i0 = (int)pMESH->m_vIndices[i];
                    int i1 = (int)pMESH->m_vIndices[i + 1];
                    int i2 = (int)pMESH->m_vIndices[i + 2];

                    auto formatIndices = [&](const int& nIndices)
                    {
                        int nIndice = nIndices + 1;
                        CString strIndices;
                        strIndices.Format("%d/%d/%d", nIndice, nIndice, nIndice);
                        return strIndices;
                    };

                    strData.Format("f %s %s %s\n", formatIndices(i0), formatIndices(i1), formatIndices(i2));
                    save.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                }
                save.Close();
            }

            if (!LoadMeshSeparately)
            {
                file.Seek((pMESH->m_dwLevel - 1) * sizeof(FLOAT), SEEK_CUR);
            }
            else
            {
                for (int i = 0; i < (int)pMESH->m_dwLevel - 1; i++)
                {
                    FLOAT fDist;
                    file.Read(&fDist, sizeof(FLOAT));
                    pMESH->m_vDist.push_back(fDist);
                }
            }

            pTRES->insert(MAPRES::value_type(dwPOS, pMESH));
            dwPOS = (DWORD)file.GetPosition();
            nIndex++;

            if(nIndex % 100 == 0)
            {
                system("cls");
                CString strProgress;
                if (!LoadMeshSeparately)
                {
                    strProgress.Format("Saving MESH: %d%%", BYTE(nIndex * 100 / nTotal));
                }
                else
                {
                    strProgress.Format("Loading MESH: %d%%", BYTE(nIndex * 100 / nTotal));
                }
                std::cout << strProgress.GetString() << std::endl;
            }
        }
    }

    void SaveTMF()
    {
        for (MAPRES::iterator it = m_mapMESH.begin(); it != m_mapMESH.end(); it++)
        {
            MeshStruct* pMESH = ((MeshStruct*)(*it).second);
            BYTE FVF = pMESH->m_bMESHType;

            CString strPath, strType;
            strType.Format(FVF == MESHTYPE_WMESH ? "WMESH" : "MESH");
            strPath.Format(".\\Data\\TMF\\%s\\%d.tmf", strType, (*it).first);
            CFile file(strPath, CFile::modeCreate | CFile::modeWrite);

            int nMeshVersion = 300;

            file.Write(&nMeshVersion, sizeof(int));
            file.Write(&pMESH->m_dwNodeCount, sizeof(DWORD));
            file.Write(&pMESH->m_fRadius, sizeof(FLOAT));
            file.Write(&pMESH->m_vCenterPoint, sizeof(D3DXVECTOR3));

            if (pMESH->m_dwNodeCount > 0)
            {
                for (SIZE_T i = 0; i < (SIZE_T)pMESH->m_dwNodeCount; i++)
                {
                    file.Write(&pMESH->m_vBones[i], sizeof(D3DXMATRIX));
                }
            }

            DWORD m_dwCount = FVF == MESHTYPE_WMESH ? (DWORD)pMESH->m_vWMESHVERTEX.size() : (DWORD)pMESH->m_vMESHVERTEX.size();
            file.Write(&m_dwCount, sizeof(DWORD));
            for (SIZE_T i = 0; i < (SIZE_T)m_dwCount; i++)
            {
                if (FVF == MESHTYPE_WMESH)
                {
                    file.Write(&pMESH->m_vWMESHVERTEX[i], sizeof(WMESHVERTEX));
                }
                else
                {
                    file.Write(&pMESH->m_vMESHVERTEX[i], sizeof(WMESHVERTEX));// should be MESHVERTEX but Zemi is retarded and they cast from WMESHVERTEX to MESHVERTEX in TachyonMesh
                }
            }

            file.Write(&pMESH->m_dwMeshCount, sizeof(DWORD));
            for (DWORD i = 0; i < pMESH->m_dwMeshCount; i++)
            {
                file.Write(&pMESH->m_dwIndicesCount, sizeof(DWORD));
                for (SIZE_T k = 0; k < (SIZE_T)pMESH->m_dwIndicesCount; k++)
                {
                    file.Write(&pMESH->m_vIndices[k], sizeof(WORD));
                }
            }

            file.Close();
        }
    }

    void SaveMESH(CString strOutput)
    {
        for (MAPRES::iterator it = m_mapMESH.begin(); it != m_mapMESH.end(); it++)
        {
            MeshStruct* pMESH = ((MeshStruct*)(*it).second);
            CString path, fsave;
            path.Format("%s\\%s\\%s", strOutput, pMESH->strFile, pMESH->m_bMESHType == MESHTYPE_WMESH ? "WMESH" : "MESH");
            fsave.Format(path + "\\%d.obj", (*it).first);
            if (!exist_directory(path.GetString()))
            {
                char* file_t = new char[path.GetLength() + 1];
                strcpy(file_t, path);
                create_directory(file_t);
            }

            CFile file(fsave, CFile::modeCreate | CFile::modeWrite);

            CString strData;
            SIZE_T i = 0;
            if (pMESH->m_bMESHType == MESHTYPE_WMESH)
            {
                SIZE_T nLength = pMESH->m_vWMESHVERTEX.size();
                for (; i < nLength; i++)
                {
                    strData.Format("v %f %f %f\n", pMESH->m_vWMESHVERTEX[i].m_fPosX, pMESH->m_vWMESHVERTEX[i].m_fPosY, pMESH->m_vWMESHVERTEX[i].m_fPosZ);
                    file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                } i = 0;
                for (; i < nLength; i++)
                {
                    strData.Format("vt %f %f\n", pMESH->m_vWMESHVERTEX[i].m_fU, pMESH->m_vWMESHVERTEX[i].m_fV);
                    file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                } i = 0;
                for (SIZE_T i = 0; i < nLength; i++)
                {
                    strData.Format("vn %f %f %f\n", pMESH->m_vWMESHVERTEX[i].m_fNormalX, pMESH->m_vWMESHVERTEX[i].m_fNormalY, pMESH->m_vWMESHVERTEX[i].m_fNormalZ);
                    file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                } i = 0;
            }
            else
            {
                SIZE_T nLength = pMESH->m_vMESHVERTEX.size();
                for (; i < nLength; i++)
                {
                    strData.Format("v %f %f %f\n", pMESH->m_vMESHVERTEX[i].m_fPosX, pMESH->m_vMESHVERTEX[i].m_fPosY, pMESH->m_vMESHVERTEX[i].m_fPosZ);
                    file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                } i = 0;
                for (; i < nLength; i++)
                {
                    strData.Format("vt %f %f\n", pMESH->m_vMESHVERTEX[i].m_fU1, pMESH->m_vMESHVERTEX[i].m_fV1);
                    file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                } i = 0;
                for (; i < nLength; i++)
                {
                    strData.Format("vt1 %f %f\n", pMESH->m_vMESHVERTEX[i].m_fU2, pMESH->m_vMESHVERTEX[i].m_fV2);
                    file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                } i = 0;
                for (; i < nLength; i++)
                {
                    strData.Format("vn %f %f %f\n", pMESH->m_vMESHVERTEX[i].m_fNormalX, pMESH->m_vMESHVERTEX[i].m_fNormalY, pMESH->m_vMESHVERTEX[i].m_fNormalZ);
                    file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
                } i = 0;
            }

            for (; i < pMESH->m_vIndices.size(); i += 3)
            {
                int i0 = (int)pMESH->m_vIndices[i];
                int i1 = (int)pMESH->m_vIndices[i + 1];
                int i2 = (int)pMESH->m_vIndices[i + 2];

                auto formatIndices = [&](const int& nIndices)
                {
                    int nIndice = nIndices + 1;
                    CString strIndices;
                    strIndices.Format("%d/%d/%d", nIndice, nIndice, nIndice);
                    return strIndices;
                };

                strData.Format("f %s %s %s\n", formatIndices(i0), formatIndices(i1), formatIndices(i2));
                file.Write((LPCTSTR)strData, strData.GetLength() * sizeof(TCHAR));
            }
            file.Close();
        }
    }

private:
    CString strPath;
    CString strGroupID;

    VECTORSTRING m_vMESHFILE;

public:
    MAPRES m_mapMESH;
};

int main()
{
    CFile file("meshcfg.txt", CFile::modeRead | CFile::typeBinary);
    DWORD dwTxtLen = (DWORD)file.GetLength();

    char* strRead = new char(dwTxtLen * sizeof(TCHAR));
    file.Read(strRead, dwTxtLen * sizeof(TCHAR));
    std::string readResult = strRead;
    delete strRead;

    int nGrpIdxStart = readResult.find("<GROUP>");
    int nGrpIdxEnd = readResult.find("</GROUP>");
    int nDataIdxStart = readResult.find("<DATA>");
    int nDataIdxEnd = readResult.find("</DATA>");
    int nIndexIdxStart = readResult.find("<INDEX>");
    int nIndexIdxEnd = readResult.find("</INDEX>");
    int nOutputIdxStart = readResult.find("<OUTPUT>");
    int nOutputIdxEnd = readResult.find("</OUTPUT");

    std::string strGroupID, strData, strIndex, strOutput;
    strGroupID = readResult.substr(nGrpIdxStart + 7, nGrpIdxEnd - nGrpIdxStart - 7);
    strData = readResult.substr(nDataIdxStart + 6, nDataIdxEnd - nDataIdxStart - 6);
    strIndex = readResult.substr(nIndexIdxStart + 7, nIndexIdxEnd - nIndexIdxStart - 7);
    strOutput = readResult.substr(nOutputIdxStart + 8, nOutputIdxEnd - nOutputIdxStart - 8);

    MeshTool app(strIndex.c_str(), strGroupID.c_str());
    app.ReadIndex(strData.c_str());
    if (LoadMeshSeparately)
    {
        std::cout << "Saving MESH!" << std::endl;
        app.SaveMESH(strOutput.c_str());
        std::cout << "Saving done!" << std::endl;
    }
    std::cin.get();
}