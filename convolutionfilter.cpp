#include "convolutionfilter.h"
#include "kernelvaluesdialog.h"
#include "sizedialog.h"

#include <QDialog>
#include <QLayout>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QVector3D>

#include <QDebug>

ConvolutionFilter::ConvolutionFilter(QObject *parent) :
    FilterInterface(parent)
{
}

bool ConvolutionFilter::setup(const QImage &img)
{
	// get kernel size
	{
		SizeDialog sizeDialog;
		if (sizeDialog.exec() != QDialog::Accepted) {
			return false;
		} else {
			QSize kernelSize = sizeDialog.kernelSize();
			mKernelWidth = kernelSize.width();
			mKernelHeight = kernelSize.height();
		}
	}
	// get kernel values
	{
		KernelValuesDialog kernelValuesDialog(mKernelWidth, mKernelHeight, qobject_cast<QWidget *>(this));
		if (kernelValuesDialog.exec() != QDialog::Accepted) {
			return false;
		}
		mKernel = kernelValuesDialog.values();
	}
	mImg = img;
	return true;
}

QImage ConvolutionFilter::apply()
{
	QImage result(mImg.size(), QImage::Format_ARGB32);
	QVector<QVector<QVector3D> > colors(mImg.height(), QVector<QVector3D>(mImg.width()));
	QVector3D min, max;
	for (int x = 0; x < result.size().width(); x++) {
		for (int y = 0; y < result.size().height(); y++) {
			QVector3D color;
			for (int j = -mKernelHeight; j < mKernelHeight + 1; j++) {
				for (int i = -mKernelWidth; i < mKernelWidth + 1; i++) {
					QPoint pos(x + i, y + j);
					// this point is outside of the image, so just skip it
					if (!mImg.rect().contains(pos)) {
						continue;
					}
					QColor rgbColor = QColor(mImg.pixel(pos));
					color += QVector3D(rgbColor.red(),
									   rgbColor.green(),
									   rgbColor.blue())
							* mKernel.at(j + mKernelHeight).at(i + mKernelWidth);
				}
			}
			colors[y][x] = color;
			if (color.x() > max.x()) {
				max.setX(color.x());
			} else if (color.x() < min.x()) {
				min.setX(color.x());
			}
			if (color.y() > max.y()) {
				max.setY(color.y());
			} else if (color.y() < min.y()) {
				min.setY(color.y());
			}
			if (color.z() > max.z()) {
				max.setZ(color.z());
			} else if (color.z() < min.z()) {
				min.setZ(color.z());
			}
		}
	}
	QVector3D spread = max - min;
	for (int y = 0; y < result.size().height(); y++) {
		for (int x = 0; x < result.size().width(); x++) {
			int red = 255 / spread.x() * (colors.at(y).at(x).x() - min.x());
			int green = 255 / spread.y() * (colors.at(y).at(x).y() - min.y());
			int blue = 255 / spread.z() * (colors.at(y).at(x).z() - min.z());
			QColor color(red, green, blue);
			if (!color.isValid()) {
				qDebug() << "invalid color at position" << x << ", " << y <<
							", it is r:" << red << ", g:" << green << ", b:" <<
							blue << "raw:" << colors.at(y).at(x);
			} else {
				result.setPixel(x, y, color.rgb());
			}
		}
	}
	qDebug() << "spread:" << spread << "max:" << max << "min:" << min;
	return result;
}

QString ConvolutionFilter::name() const
{
	return "Convolution";
}