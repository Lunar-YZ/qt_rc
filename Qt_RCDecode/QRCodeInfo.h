#ifndef QR_CODE_INFO_H
#define QR_CODE_INFO_H

typedef struct tagRS_BLOCKINFO
{
	int ncRSBlock;		// �������
	int ncAllCodeWord;	// ����������
	int ncDataCodeWord;	// ������������������-RS��������

} RS_BLOCKINFO, *LPRS_BLOCKINFO;

//QRcode�汾�����Ϣ
typedef struct tagQR_VERSIONINFO
{
	int nVersionNo;	   // �汾��1-40
	int ncAllCodeWord; // ��������
	int ncDataCodeWord[4];	// �������֣�������-RS���֣�

	int ncAlignPoint;	// У������
	int nAlignPoint[6];	// У��ͼ����������

	RS_BLOCKINFO RS_BlockInfo1[4]; // �������Ϣ
	RS_BLOCKINFO RS_BlockInfo2[4]; // �������Ϣ

} QR_VERSIONINFO, *LPQR_VERSIONINFO;
		

int VersionInfoBit[];
int FormatInfoBit[];
int nIndicatorLenNumeral[];
int nIndicatorLenAlphabet[];
int nIndicatorLen8Bit[];
int nIndicatorLenHanzi[];
QR_VERSIONINFO QR_VersonInfo[];

#endif
