#include "dialog.h"
#include "ui_dialog.h"

#include <QFileDialog>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog),m_pDecodeImage(NULL)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_pushButton_clicked()
{
	// ��ѡ���ļ��Ի���
	QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),
		"", tr("Images (*.png *.bmp *.jpg)"));

	if (fileName.isNull())
		return;

	// ��ʾ��ǰѡ���ļ���·��
	ui->path_edit->setText(fileName);

	// ��ʾѡ���ͼƬ
	m_EncodeImage.load(fileName);
	if (!m_EncodeImage.isNull())
	{
		QImage scale_image = m_EncodeImage.scaled(ui->image_label->width(),
			ui->image_label->height());
		ui->image_label->setPixmap(QPixmap::fromImage(scale_image));
	}

	// �����ά��ͼƬ
	m_pDecodeImage = new QRcodeImage(fileName);
	m_pDecodeImage->GetImageData();

	int version = m_pDecodeImage->finderPattern->m_version;
	int code_size = version * 4 +17;
	for(int i=0; i<code_size; i++)
	{
		for(int j=0; j<code_size; j++)
		{
			m_CodeData[i][j]=m_pDecodeImage->bitMatrix[i][j];
		}
	}

	ContentDecoder decoder;
	decoder.DecodeData(code_size, version, m_CodeData);

	// ��ʾ������Ϣ���汾�š�����ȼ�)
	QString level;
	switch (decoder.m_nLevel)
	{
	case QR_LEVEL_L:level="L(%7)";break;
	case QR_LEVEL_M:level="M(%15)";break;
	case QR_LEVEL_Q:level="Q(%25)";break;
	case QR_LEVEL_H:level="H(%30)";break;
	}
	ui->level_label->setText(level);
	ui->version_label->setText(QString::number(decoder.m_nVersion));
	ui->decode_info_edit->setText(decoder.m_strData);
}
