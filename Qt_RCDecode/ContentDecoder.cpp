// ContentDecoder.cpp: implementation of the ContentDecoder class.
//
//////////////////////////////////////////////////////////////////////
#include "ContentDecoder.h"
#include "QRCodeInfo.h"

#include <QMessageBox>
#include <QDialog>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ContentDecoder::ContentDecoder()
{

}

ContentDecoder::~ContentDecoder()
{

}

/////////////////////////////////////////////////////////////////////////////
//���ã�����
//�������汾�ţ�Ĭ��0����ͼƬ���ݣ��ַ���
//����ֵ���ɹ�����TRUE��ʧ�ܷ���FALSE

bool ContentDecoder::DecodeData(int nCodeSize,int nVersion,unsigned char CodeData[MAX_MODULESIZE][MAX_MODULESIZE])
{
	int i,j;

	m_nCodeSize=nCodeSize;
	m_nVersion=nVersion;
	for(i=0;i<nCodeSize;i++)
		for(j=0;j<nCodeSize;j++)
			m_CodeData[i][j]=CodeData[i][j];

	//��ȡ�汾��Ϣ��7~40�а汾��Ϣ��
	//if(m_nVersion>=7)
	//{
	//	m_nVersion=GetVersionInfo();
	//}

	//��ȡ��ʽ��Ϣ
	int nFormatData=GetFormatInfo();

	m_nLevel=((nFormatData & 0x18) >> 3);//������ǰ��λ
	switch(m_nLevel)
	{
	case 1: m_nLevel=QR_LEVEL_L;break;
	case 0: m_nLevel=QR_LEVEL_M;break;
	case 3: m_nLevel=QR_LEVEL_Q;break;
	case 2: m_nLevel=QR_LEVEL_H;break;
	}
	m_nMaskingNo=(nFormatData & 0x07);//��Ĥͼ�βο�������λ

	//ȥ����Ĥͼ��
	for(i=0;i<m_nCodeSize;++i)
	{
		for(j=0;j<m_nCodeSize;++j)
		{
			bool bMask;
			switch (m_nMaskingNo)
			{
			case 0:
				bMask=((i+j)%2==0);break;
			case 1:
				bMask=(i%2==0);break;
			case 2:
				bMask=(j%3==0);break;
			case 3:
				bMask=((i+j) % 3 == 0);break;
			case 4:
				bMask=(((i/2)+(j/3))%2==0);break;
			case 5:
				bMask=(((i*j)%2)+((i*j)%3)==0);break;
			case 6:
				bMask=((((i*j)%2)+((i* j)%3))%2==0);break;
			default: // case 7:
				bMask=((((i*j)%3)+((i+j)%2))%2==0);break;
			}
			m_CodeData[j][i] = (unsigned char)(m_CodeData[j][i] ^ bMask);
		}
	}

	//��Ƿ�����λ��
	SetFinderPattern();
	if(m_nVersion>1)
		SetAlignmentPattern();
	SetFormatPattern();
	SetVersionPattern();

	//��ȡ����λ�ã��õ�����λ��
	GetCodeWord();

	//����
	CorrectDataBlocks();

	//�����������֣��õ��ַ���
	ParseDataCodeWord();

	return true;
}

//��ȡ����
void ContentDecoder::GetCodeWord()
{
	//�汾��Ӧ��������
	m_ncAllCodeWord=QR_VersonInfo[m_nVersion].ncAllCodeWord;
	memset(m_byAllCodeWord, 0, m_ncAllCodeWord);

	//�����������ֺ;�������
	int x=m_nCodeSize;
	int y=m_nCodeSize-1;

	int nCoef_x=1; //X�᷽��
	int nCoef_y=1; //y�᷽��

	int i,j;

	for(i=0;i<m_ncAllCodeWord;i++)
	{
		for(j=0;j<8;j++)
		{
			do
			{
				x+=nCoef_x;
				nCoef_x*=-1;
				if (nCoef_x< 0)
				{
					y+=nCoef_y;
					if(y<0||y==m_nCodeSize)
					{
						y=(y<0)?0:m_nCodeSize-1;
						nCoef_y*=-1;
						x-=2;
						if (x == 6)
							x--;
					}
				}
			}
			while(m_CodeData[x][y]==NOT_DATA); //������λ�ù���

			m_byAllCodeWord[i] += m_CodeData[x][y] << (7 - j);//���ݶ���
		}
	}
}

//����������������
void ContentDecoder::ParseDataCodeWord()
{
	int i,j;

	m_nVersionGroup = m_nVersion >= 27 ? QR_VRESION_L : (m_nVersion >= 10 ? QR_VRESION_M : QR_VRESION_S);
	m_nIndex=0;

	//int ncComplete;//�Ѷ�ȡ�ֽ���
	unsigned short wData;
	unsigned short wIndicator;
	//int i,j;
	for(i=0;i<m_ncDataCodeWord && m_nIndex != -1; i++)
	{
		wData=GetBitStream(4);
		if(wData==0)//0000b��ֹ
		{
			//AfxMessageBox("��ֹ��");
			break;
		}
		else if(wData==1)//0001b����ģʽ
		{
			wIndicator=GetBitStream(nIndicatorLenNumeral[m_nVersionGroup]);
			for(j=0;j<wIndicator;j+=3)
			{
				QString temp;
				if(j<wIndicator-2)
				{
					wData=GetBitStream(10);
					temp = QString::number(wData/100);
					m_strData+=temp;
					temp = QString::number((wData%100)/10);
					m_strData+=temp;
					temp = QString::number(wData%10);
					m_strData+=temp;
				}
				else if (j == wIndicator-2)
				{
					//ʣ��2��
					wData=GetBitStream(7);
					temp = QString::number(wData/10);
					m_strData+=temp;
					temp = QString::number(wData%10);
					m_strData+=temp;
				}
				else if (j == wIndicator-1)
				{
					//ʣ��1��
					wData=GetBitStream(4);
					temp = QString::number(wData);
					m_strData+=temp;
				}
			}

		}
		else if(wData==2)//0010b��ĸ����
		{
			wIndicator=GetBitStream(nIndicatorLenAlphabet[m_nVersionGroup]);
			for(j=0; j<wIndicator;j+=2)
			{
				if(j<wIndicator-1)
				{
					wData=GetBitStream(11);
					m_strData+=BinaryToAlphabet((unsigned char)(wData/45));
					m_strData+=BinaryToAlphabet((unsigned char)(wData%45));
				}
				else
				{
					//ʣ��1��
					wData=GetBitStream(6);
					m_strData+=BinaryToAlphabet((unsigned char)wData);
				}
			}
		}
		else if(wData==4)//0100b8λ�ֽ�
		{
			wIndicator=GetBitStream(nIndicatorLen8Bit[m_nVersionGroup]);
			for(j=0;j<wIndicator;j++)
			{
				wData=GetBitStream(8);
				m_strData+=(char)wData;
			}
		}
		else if(wData==13)//1101b�й�����
		{
			//GetBitStream(4);
			wIndicator=GetBitStream(nIndicatorLenHanzi[m_nVersionGroup]);
			for(j=0;j<wIndicator;j++)
			{
				wData=GetBitStream(13);

				//�����ַ�ת��
				unsigned char by1,by2;
				by1=(wData/0x60)<<8;
				if(by1<=0x09)
					by1+=0xA1;
				else//>=0x0A
					by1+=0xA6;
				by2=wData%0x60;
				by2+=0xA1;
				m_strData+=(char)by1;
				m_strData+=(char)by2;
			}
		}
		else//����
		{
			QMessageBox::critical(NULL, "Error", "��֧�ֵ��ַ���");
			exit(0);
		}
	}
}

//��ȡλ����m_nIndex��ʼ��ncDataλ������
unsigned short ContentDecoder::GetBitStream(int ncData)
{
	unsigned short wData=0;
	//����������
	if(m_nIndex==-1 || m_nIndex+ncData>m_ncDataCodeWord*8)
	{
		m_nIndex=-1;
		return -1;
	}

	//��ȡλ
	for(int i=0;i<ncData;i++)
	{
		if(m_byDataCodeWord[(m_nIndex+i)/8] & (1 << (7-((m_nIndex+i)%8))))
		{
			wData |= (1<<(ncData-i-1));
		}
	}
	m_nIndex+=ncData;
	return wData;

}

//��ĸ����ģʽ�ı���/�����
char ContentDecoder::BinaryToAlphabet(unsigned char by)
{
	if(by>=0&&by<=9)
		return by+'0';
	else if(by>=10&&by<=35)
		return by-10+'A';
	else if(by==36)return ' ';
	else if(by==37)return '$';
	else if(by==38)return '%';
	else if(by==39)return '*';
	else if(by==40)return '+';
	else if(by==41)return '-';
	else if(by==42)return '.';
	else if(by==43)return '/';
	else return ':';//by==44
}

//���̽��ͼ��
void ContentDecoder::SetFinderPattern()
{
	int i,j;

	//����
	for(i=0;i<8;i++)
		for(j=0;j<8;j++)
			m_CodeData[i][j]=NOT_DATA;
	//��
	for(i=m_nCodeSize-1;i>=m_nCodeSize-8;i--)
		for(j=0;j<8;j++)
			m_CodeData[i][j]=NOT_DATA;
	//��
	for(i=0;i<8;i++)
		for(j=m_nCodeSize-1;j>=m_nCodeSize-8;j--)
			m_CodeData[i][j]=NOT_DATA; 
}

//��Ƕ�λͼ�Ρ�У��ͼ��
void ContentDecoder::SetAlignmentPattern()
{
	int i,j;

	//��λͼ�Σ�6�ĺ�������
	for(i=0;i<m_nCodeSize;i++)
	{
		m_CodeData[6][i]=NOT_DATA;
		m_CodeData[i][6]=NOT_DATA;
	}

	//У��ͼ��
	for(i=0;i<QR_VersonInfo[m_nVersion].ncAlignPoint;i++)
	{
		int u,v;
		//6�������ϵ�У��
		if(i<QR_VersonInfo[m_nVersion].ncAlignPoint-1)
		{
			for(u=-2;u<=2;u++)
				for(v=-2;v<=2;v++)
					m_CodeData[6+u][QR_VersonInfo[m_nVersion].nAlignPoint[i]+v]=NOT_DATA;
			for(u=-2;u<=2;u++)
				for(v=-2;v<=2;v++)
					m_CodeData[QR_VersonInfo[m_nVersion].nAlignPoint[i]+u][6+v]=NOT_DATA;
		}

		//������У��ͼ��
		for(j=0;j<QR_VersonInfo[m_nVersion].ncAlignPoint;j++)
		{
			for(u=-2;u<=2;u++)
				for(v=-2;v<=2;v++)
					m_CodeData[QR_VersonInfo[m_nVersion].nAlignPoint[i]+u][QR_VersonInfo[m_nVersion].nAlignPoint[j]+v]=NOT_DATA;
		}
	}
}

//��Ǹ�ʽ��Ϣ
void ContentDecoder::SetFormatPattern()
{
	int i;
	//����
	for(i=0;i<=8;i++)
	{
		m_CodeData[i][8]=NOT_DATA;
		m_CodeData[8][i]=NOT_DATA;
	}
	//��
	for(i=m_nCodeSize-1;i>=m_nCodeSize-8;i--)
	{
		m_CodeData[i][8]=NOT_DATA;
	}
	//��
	for(i=m_nCodeSize-1;i>=m_nCodeSize-8;i--)
	{
		m_CodeData[8][i]=NOT_DATA;
	}

}

//��ǰ汾��Ϣ
void ContentDecoder::SetVersionPattern()
{
	if(m_nVersion>=7)
	{
		int i,j;
		//��
		for(i=m_nCodeSize-11;i<=m_nCodeSize-9;i++)
			for(j=0;j<=5;j++)
				m_CodeData[i][j]=NOT_DATA;
		//��
		for(i=0;i<=5;i++)
			for(j=m_nCodeSize-11;j<=m_nCodeSize-9;j++)
				m_CodeData[i][j]=NOT_DATA;
	}

}

//��ȡ�汾��Ϣ�����ذ汾��
int ContentDecoder::GetVersionInfo()
{
	int i,j,k;

	unsigned char VersionInfo[18];
	k=0;
	//���� --- �汾��Ϣ1
	for(j=0;j<=5;j++)
		for(i=m_nCodeSize-11;i<=m_nCodeSize-9;i++)
			VersionInfo[k++]=m_CodeData[i][j];

    //������
	int errorCount=0,versionBase;
    for(versionBase=0;versionBase<=33;versionBase++)
	{
        errorCount=0;
        for(j=0;j<18;j++)
		{
            if((VersionInfo[j]^(VersionInfoBit[versionBase]>>j))%2==1)
                errorCount++;
        }
        if(errorCount<=3)break;
    }
    if(errorCount<=3)
        return 7+versionBase;
    else
	{
		k=0;
		//���� --- �汾��Ϣ2
		for(i=0;i<=5;i++)
			for(j=m_nCodeSize-11;j<=m_nCodeSize-9;j++)
				VersionInfo[k++]=m_CodeData[i][j];
        
		//������
		errorCount=0;
        for(versionBase=0;versionBase<=33;versionBase++)
		{
            errorCount=0;
            for(j=0;j<18;j++)
			{
                if((VersionInfo[j]^(VersionInfoBit[versionBase]>>j))%2==1)
                    errorCount++;
            }
            if(errorCount<=3)break;
        }
        if(errorCount<=3)
            return 7+versionBase;
        else     
		{
			QMessageBox::critical(NULL, "Error", "�汾��Ϣ����ʧ��");
			exit(0);  
		}
	}

	return 0;
}


//��ȡ��ʽ��Ϣ�����ؾ�������Ĥ�ŵ�������Ϣ
int ContentDecoder::GetFormatInfo()
{
	int i,j;
	unsigned char FormatInfo[15];

	//��ȡ��ʽ��Ϣ1(����)
	for (i=0;i<=5;i++)
		FormatInfo[i]=m_CodeData[8][i];
	FormatInfo[6]=m_CodeData[8][7];
	FormatInfo[7]=m_CodeData[8][8];
	FormatInfo[8]=m_CodeData[7][8];
	for (i=9;i<=14;i++)
		FormatInfo[i]=m_CodeData[14-i][8];

	//0x5412 101010000010010b ��ʽ��Ϣ��Ĥȥ��
	for(i=0;i<15;i++)
	{
		j=(0x5412 & (1<<i))?1:0;
		FormatInfo[i]^=j;
	}
		
	int errorCount=0,Info;
    for(Info=0;Info<32/*FormatInfoBit.length*/;Info++)
	{
        errorCount=0;
        for(j=0;j<15;j++)
		{
            if((FormatInfo[j]^(FormatInfoBit[Info]>>j))%2==1)
                errorCount++;
        }
        if(errorCount<=3)break;
    }
    if(errorCount<=3)
        return Info;
    else
	{
		//��ȡ��ʽ��Ϣ2 (���ϡ�����)
		for(i=0;i<=7;i++)
			FormatInfo[i]=m_CodeData[m_nCodeSize-1-i][8];
		for(i=8;i<=14;i++)
			FormatInfo[i]=m_CodeData[8][m_nCodeSize-15+i];
		
		//0x5412 101010000010010b ��ʽ��Ϣ��Ĥȥ��
		for(i=0;i<15;i++)
		{
			j=(0x5412 & (1<<i))?1:0;
			FormatInfo[i]^=j;
		}

		int errorCount=0,Info;
		for(Info=0;Info<32/*FormatInfoBit.length*/;Info++)
		{
			errorCount=0;
			for(j=0;j<15;j++)
			{
				if((FormatInfo[j]^(FormatInfoBit[Info]>>j))%2==1)
					errorCount++;
			}
			if(errorCount<=3)break;
		}
		if(errorCount<=3)
			return Info;
		else
		{
			QMessageBox::critical(NULL, "Error", "��ʽ��Ϣ����ʧ��");
			exit(0);
		}
	}
}

//����
void ContentDecoder::CorrectDataBlocks()
{
	int i,j,k;

	int numSucceededCorrections = 0;
	int numCorrectionFailures = 0;

	m_ncAllCodeWord=QR_VersonInfo[m_nVersion].ncAllCodeWord;//int dataCapacity = qrCodeSymbol.getDataCapacity();


	m_ncDataCodeWord = QR_VersonInfo[m_nVersion].ncDataCodeWord[m_nLevel];//...
	m_ncRSCodeWord=m_ncAllCodeWord-m_ncDataCodeWord;//int numErrorCollectionCode = qrCodeSymbol.getNumErrorCollectionCode();

	int ncBlock1 = QR_VersonInfo[m_nVersion].RS_BlockInfo1[m_nLevel].ncRSBlock;//...
	int ncBlock2 = QR_VersonInfo[m_nVersion].RS_BlockInfo2[m_nLevel].ncRSBlock;//...
	int ncBlockSum = ncBlock1 + ncBlock2;//int numRSBlocks = qrCodeSymbol.getNumRSBlocks();

	int eccPerRSBlock = m_ncRSCodeWord / ncBlockSum;

	if (ncBlockSum == 1)
	{
		RsDecode corrector = RsDecode(eccPerRSBlock / 2);
		int ret = corrector.decode(m_byAllCodeWord,m_ncAllCodeWord);
		if (ret > 0)
			numSucceededCorrections += ret;
		else if (ret < 0)
			numCorrectionFailures++;
		
		for(i=0;i<m_ncDataCodeWord;i++)
			m_byDataCodeWord[i]=m_byAllCodeWord[i];
	}
	else
	{ //interleave data blocks because symbol has 2 or more RS blocks
		int * dataBlocks = new int[m_ncAllCodeWord];
		memset(dataBlocks, 0, m_ncAllCodeWord);
		int numLongerRSBlocks = m_ncAllCodeWord % ncBlockSum;
		if (numLongerRSBlocks == 0)
		{ //symbol has only 1 type of RS block
			int lengthRSBlock = m_ncAllCodeWord / ncBlockSum;
			unsigned char ** RSBlocks = new unsigned char * [ncBlockSum];//...
			for(k=0;k<ncBlockSum;k++)
			{
				RSBlocks[k] = new unsigned char [lengthRSBlock];
				memset(RSBlocks[k], 0, lengthRSBlock);
			}//int[][] RSBlocks = new int[ncBlockSum][lengthRSBlock];
		
			//obtain RS blocks
			for (i = 0; i < ncBlockSum; i++)
			{
				for (j = 0; j < lengthRSBlock; j++)
				{
					RSBlocks[i][j] = m_byAllCodeWord[j * ncBlockSum + i];
				}
				RsDecode corrector = RsDecode(eccPerRSBlock / 2);
				int ret = corrector.decode(RSBlocks[i],lengthRSBlock);
				if (ret > 0)
					numSucceededCorrections += ret;
				else if (ret < 0)
					numCorrectionFailures++;
			}
		
			//obtain only data part
			int p = 0;
			for (i = 0; i < ncBlockSum; i++)
			{
				for (j = 0; j < lengthRSBlock - eccPerRSBlock; j++)
				{
					dataBlocks[p++] = RSBlocks[i][j];
				}
			}
		}
		else
		{ //symbol has 2 types of RS blocks
			int lengthShorterRSBlock = m_ncAllCodeWord / ncBlockSum;
			int lengthLongerRSBlock = m_ncAllCodeWord / ncBlockSum + 1;
			int numShorterRSBlocks = ncBlockSum - numLongerRSBlocks;

			unsigned char ** shorterRSBlocks = new unsigned char * [numShorterRSBlocks];//...
			for(k=0;k<numShorterRSBlocks;k++)
			{
				shorterRSBlocks[k] = new unsigned char [lengthShorterRSBlock];
				memset(shorterRSBlocks[k], 0, lengthShorterRSBlock);
			}//int[][] shorterRSBlocks = new int[numShorterRSBlocks][lengthShorterRSBlock];

			unsigned char ** longerRSBlocks = new unsigned char * [numLongerRSBlocks];//...
			for(k=0;k<numLongerRSBlocks;k++)
			{
				longerRSBlocks[k] = new unsigned char [lengthLongerRSBlock];
				memset(longerRSBlocks[k], 0, lengthLongerRSBlock);
			}//int[][] longerRSBlocks = new int[numLongerRSBlocks][lengthLongerRSBlock];
		
			for (int i = 0; i < ncBlockSum; i++)
			{
				if (i < numShorterRSBlocks)
				{ //get shorter RS Block(s)
					int mod = 0;
					for (int j = 0; j < lengthShorterRSBlock; j++)
					{
						if (j == lengthShorterRSBlock - eccPerRSBlock)
							mod = numLongerRSBlocks;
						shorterRSBlocks[i][j] = m_byAllCodeWord[j * ncBlockSum + i + mod];
					}
					//canvas.println("eccPerRSBlock(shorter)=" + eccPerRSBlock );
					RsDecode corrector = RsDecode(eccPerRSBlock / 2);
					int ret = corrector.decode(shorterRSBlocks[i],lengthShorterRSBlock);
					if (ret > 0)
						numSucceededCorrections += ret;
					else if (ret < 0)
						numCorrectionFailures++;
				}
				else
				{ 	//get longer RS Blocks
					int mod = 0;
					for (int j = 0; j < lengthLongerRSBlock; j++)
					{
						if (j == lengthShorterRSBlock - eccPerRSBlock)
							mod = numShorterRSBlocks;
						longerRSBlocks[i - numShorterRSBlocks][j] = m_byAllCodeWord[j * ncBlockSum + i - mod];
					}
					//canvas.println("eccPerRSBlock(longer)=" + eccPerRSBlock );
					RsDecode corrector = RsDecode(eccPerRSBlock / 2);
					int ret = corrector.decode(longerRSBlocks[i-numShorterRSBlocks],lengthLongerRSBlock);
					if (ret > 0)
						numSucceededCorrections += ret;
					else if (ret < 0)
						numCorrectionFailures++;
				}
			}
		
			int p = 0;
			for (int i = 0; i < ncBlockSum; i++)
			{
				if (i < numShorterRSBlocks)
				{
					for (int j = 0; j < lengthShorterRSBlock - eccPerRSBlock; j++)
					{
						dataBlocks[p++] = shorterRSBlocks[i][j];
					}
				}
				else
				{
					for (int j = 0; j < lengthLongerRSBlock - eccPerRSBlock; j++)
					{
						dataBlocks[p++] = longerRSBlocks[i - numShorterRSBlocks][j];
					}
				}
			}
		}
		/*if (numSucceededCorrections > 0)
		{
			char s[10];
			itoa(numSucceededCorrections,s,10);
			AfxMessageBox(s);//" data errors corrected successfully."numSucceededCorrections
		}
		else
			AfxMessageBox("No errors found.");*/

		for(i=0;i<m_ncDataCodeWord;i++)
			m_byDataCodeWord[i]=dataBlocks[i];
	}
}
