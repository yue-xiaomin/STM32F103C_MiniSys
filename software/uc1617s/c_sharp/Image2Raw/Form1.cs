using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.IO;
using System.Text;
using System.Windows.Forms;

namespace UC1617S_Image2Raw
{
    public partial class Form1 : Form
    {
        private string imagePath = "";
        private byte[] rawData = null;
        private int outW, outH;
        private Bitmap srcImage = null;

        public Form1()
        {
            InitializeComponent();
        }

        /// <summary>
        /// 浏览选择图片
        /// </summary>
        private void btnBrowse_Click(object sender, EventArgs e)
        {
            OpenFileDialog dlg = new OpenFileDialog();
            dlg.Filter = "图片文件|*.bmp;*.png;*.jpg;*.jpeg;*.gif;*.tiff|所有文件|*.*";

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                imagePath = dlg.FileName;
                txtFile.Text = Path.GetFileName(imagePath);

                try
                {
                    // 加载原图
                    if (srcImage != null) srcImage.Dispose();
                    srcImage = new Bitmap(imagePath);

                    txtW.Text = srcImage.Width.ToString();
                    txtH.Text = srcImage.Height.ToString();

                    picOrig.Image = Image.FromFile(imagePath);
                    picPreview.Image = null;
                    rawData = null;
                    lblSizeInfo.Text = "";

                    // 立即预览
                    DoConvert();
                }
                catch (Exception ex)
                {
                    MessageBox.Show("无法读取图片: " + ex.Message, "错误");
                }
            }
        }

        /// <summary>
        /// 滑块拖动 — 实时预览
        /// </summary>
        private void trackTh_Scroll(object sender, EventArgs e)
        {
            // 确保 th1 < th2 < th3
            if (trackTh1.Value >= trackTh2.Value)
                trackTh1.Value = trackTh2.Value - 1;
            if (trackTh2.Value >= trackTh3.Value)
                trackTh2.Value = trackTh3.Value - 1;

            // 更新数值标签
            lblTh1Val.Text = trackTh1.Value.ToString();
            lblTh2Val.Text = trackTh2.Value.ToString();
            lblTh3Val.Text = trackTh3.Value.ToString();

            // 实时转换预览
            if (srcImage != null)
            {
                DoConvert();
            }
        }

        /// <summary>
        /// 转换图片为 2bit 像素数据 + 预览
        /// </summary>
        private void DoConvert()
        {
            if (srcImage == null) return;

            outW = int.Parse(txtW.Text);
            outH = int.Parse(txtH.Text);
            int th1 = trackTh1.Value;
            int th2 = trackTh2.Value;
            int th3 = trackTh3.Value;
            int bpr = (outW + 3) / 4;

            rawData = new byte[bpr * outH];

            // 缩放到目标尺寸
            using (Bitmap bmp = new Bitmap(outW, outH))
            using (Graphics g = Graphics.FromImage(bmp))
            {
                g.InterpolationMode = InterpolationMode.HighQualityBicubic;
                g.DrawImage(srcImage, 0, 0, outW, outH);

                for (int y = 0; y < outH; y++)
                {
                    for (int x = 0; x < outW; x++)
                    {
                        Color c = bmp.GetPixel(x, y);
                        int gray = (c.R * 77 + c.G * 150 + c.B * 29) >> 8;

                        uint pixel;
                        if (gray < th1) pixel = 3;
                        else if (gray < th2) pixel = 2;
                        else if (gray < th3) pixel = 1;
                        else pixel = 0;

                        int byteIdx = x >> 2;
                        int bitShift = (x & 3) << 1;
                        rawData[y * bpr + byteIdx] |= (byte)(pixel << bitShift);
                    }
                }
            }

            // 生成预览图
            UpdatePreview(bpr);

            // 更新大小信息
            lblSizeInfo.Text = "数据大小: " + rawData.Length + " 字节  |  "
                + outW + "x" + outH + "  |  " + bpr + " 字节/行";
        }

        /// <summary>
        /// 更新预览图
        /// </summary>
        private void UpdatePreview(int bpr)
        {
            Bitmap preview = new Bitmap(outW, outH);

            for (int y = 0; y < outH; y++)
            {
                for (int x = 0; x < outW; x++)
                {
                    uint px = (uint)((rawData[y * bpr + (x >> 2)] >> ((x & 3) << 1)) & 3);
                    int g2;
                    switch (px)
                    {
                        case 0: g2 = 255; break;   // 白
                        case 1: g2 = 170; break;   // 浅灰
                        case 2: g2 = 85; break;    // 深灰
                        default: g2 = 0; break;    // 黑
                    }
                    preview.SetPixel(x, y, Color.FromArgb(g2, g2, g2));
                }
            }

            if (picPreview.Image != null)
                picPreview.Image.Dispose();
            picPreview.Image = preview;
        }

        /// <summary>
        /// 转换按钮 (其实已经实时转换了, 这里只是弹个提示)
        /// </summary>
        private void btnConvert_Click(object sender, EventArgs e)
        {
            if (srcImage == null)
            {
                MessageBox.Show("请先选择图片文件", "提示");
                return;
            }
            DoConvert();
            MessageBox.Show("转换完成\n尺寸: " + outW + "x" + outH
                + "\n数据: " + rawData.Length + " 字节", "完成");
        }

        /// <summary>
        /// 生成 C 数组字符串
        /// </summary>
        private string GenerateCArray()
        {
            if (rawData == null) DoConvert();
            if (rawData == null) return "";

            int bpr = (outW + 3) / 4;
            string arrName = txtName.Text;

            StringBuilder sb = new StringBuilder();
            sb.AppendLine("/* " + outW + "x" + outH + ", " + rawData.Length + " bytes, " + bpr + " bytes/row */");
            sb.AppendLine("/* 2bit/pixel: 0=白, 1=浅灰, 2=深灰, 3=黑 */");
            sb.AppendLine("static const uint8_t " + arrName + "[] = {");

            for (int y = 0; y < outH; y++)
            {
                sb.Append("    ");
                for (int x = 0; x < bpr; x++)
                {
                    sb.Append("0x" + rawData[y * bpr + x].ToString("X2") + ",");
                }
                sb.AppendLine();
            }

            sb.AppendLine("};");
            return sb.ToString();
        }

        /// <summary>
        /// 预览数据
        /// </summary>
        private void btnPreview_Click(object sender, EventArgs e)
        {
            try
            {
                txtPreview.Text = GenerateCArray();
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误");
            }
        }

        /// <summary>
        /// 导出 .h 文件
        /// </summary>
        private void btnExportH_Click(object sender, EventArgs e)
        {
            ExportFile(".h");
        }

        /// <summary>
        /// 导出 .c 文件
        /// </summary>
        private void btnExportC_Click(object sender, EventArgs e)
        {
            ExportFile(".c");
        }

        /// <summary>
        /// 导出文件通用
        /// </summary>
        private void ExportFile(string ext)
        {
            try
            {
                if (rawData == null) DoConvert();
                if (rawData == null) return;

                string arrName = txtName.Text;

                SaveFileDialog dlg = new SaveFileDialog();
                dlg.FileName = arrName + ext;
                dlg.Filter = ext == ".h" ? "C头文件|*.h" : "C源文件|*.c";

                if (dlg.ShowDialog() != DialogResult.OK) return;

                StringBuilder sb = new StringBuilder();

                if (ext == ".h")
                {
                    string guard = Path.GetFileName(dlg.FileName).Replace('.', '_').ToUpper();
                    sb.AppendLine("#ifndef __" + guard);
                    sb.AppendLine("#define __" + guard);
                    sb.AppendLine();
                    sb.AppendLine("#include <stdint.h>");
                    sb.AppendLine();
                }

                sb.Append(GenerateCArray());
                sb.AppendLine();

                if (ext == ".h")
                {
                    string guard = Path.GetFileName(dlg.FileName).Replace('.', '_').ToUpper();
                    sb.AppendLine("#endif /* __" + guard + " */");
                }

                File.WriteAllText(dlg.FileName, sb.ToString(), Encoding.UTF8);
                MessageBox.Show("已保存: " + dlg.FileName + "\n数据大小: " + rawData.Length + " 字节", "完成");
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message, "错误");
            }
        }

        /// <summary>
        /// 复制到剪贴板
        /// </summary>
        private void btnCopy_Click(object sender, EventArgs e)
        {
            if (!string.IsNullOrEmpty(txtPreview.Text))
            {
                Clipboard.SetText(txtPreview.Text);
                MessageBox.Show("已复制到剪贴板", "提示");
            }
            else
            {
                MessageBox.Show("请先点击 预览数据", "提示");
            }
        }
    }
}
