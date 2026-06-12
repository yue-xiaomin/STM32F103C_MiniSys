namespace UC1617S_Image2Raw
{
    partial class Form1
    {
        private System.ComponentModel.IContainer components = null;

        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        private void InitializeComponent()
        {
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.lblSizeInfo = new System.Windows.Forms.Label();
            this.btnCopy = new System.Windows.Forms.Button();
            this.btnExportC = new System.Windows.Forms.Button();
            this.btnExportH = new System.Windows.Forms.Button();
            this.btnPreview = new System.Windows.Forms.Button();
            this.btnConvert = new System.Windows.Forms.Button();
            this.txtPreview = new System.Windows.Forms.TextBox();
            this.lblThInfo = new System.Windows.Forms.Label();
            this.lblTh3Val = new System.Windows.Forms.Label();
            this.lblTh2Val = new System.Windows.Forms.Label();
            this.lblTh1Val = new System.Windows.Forms.Label();
            this.trackTh3 = new System.Windows.Forms.TrackBar();
            this.trackTh2 = new System.Windows.Forms.TrackBar();
            this.trackTh1 = new System.Windows.Forms.TrackBar();
            this.lblTh3 = new System.Windows.Forms.Label();
            this.lblTh2 = new System.Windows.Forms.Label();
            this.lblTh1 = new System.Windows.Forms.Label();
            this.picOrig = new System.Windows.Forms.PictureBox();
            this.picPreview = new System.Windows.Forms.PictureBox();
            this.lblPreview = new System.Windows.Forms.Label();
            this.lblOrig = new System.Windows.Forms.Label();
            this.txtName = new System.Windows.Forms.TextBox();
            this.lblName = new System.Windows.Forms.Label();
            this.txtH = new System.Windows.Forms.TextBox();
            this.lblH = new System.Windows.Forms.Label();
            this.txtW = new System.Windows.Forms.TextBox();
            this.lblW = new System.Windows.Forms.Label();
            this.btnBrowse = new System.Windows.Forms.Button();
            this.txtFile = new System.Windows.Forms.TextBox();
            this.lblFile = new System.Windows.Forms.Label();
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackTh3)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackTh2)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackTh1)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.picOrig)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.picPreview)).BeginInit();
            this.SuspendLayout();
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.lblSizeInfo);
            this.groupBox1.Controls.Add(this.btnCopy);
            this.groupBox1.Controls.Add(this.btnExportC);
            this.groupBox1.Controls.Add(this.btnExportH);
            this.groupBox1.Controls.Add(this.btnPreview);
            this.groupBox1.Controls.Add(this.btnConvert);
            this.groupBox1.Controls.Add(this.txtPreview);
            this.groupBox1.Controls.Add(this.lblThInfo);
            this.groupBox1.Controls.Add(this.lblTh3Val);
            this.groupBox1.Controls.Add(this.lblTh2Val);
            this.groupBox1.Controls.Add(this.lblTh1Val);
            this.groupBox1.Controls.Add(this.trackTh3);
            this.groupBox1.Controls.Add(this.trackTh2);
            this.groupBox1.Controls.Add(this.trackTh1);
            this.groupBox1.Controls.Add(this.lblTh3);
            this.groupBox1.Controls.Add(this.lblTh2);
            this.groupBox1.Controls.Add(this.lblTh1);
            this.groupBox1.Controls.Add(this.picOrig);
            this.groupBox1.Controls.Add(this.picPreview);
            this.groupBox1.Controls.Add(this.lblPreview);
            this.groupBox1.Controls.Add(this.lblOrig);
            this.groupBox1.Controls.Add(this.txtName);
            this.groupBox1.Controls.Add(this.lblName);
            this.groupBox1.Controls.Add(this.txtH);
            this.groupBox1.Controls.Add(this.lblH);
            this.groupBox1.Controls.Add(this.txtW);
            this.groupBox1.Controls.Add(this.lblW);
            this.groupBox1.Controls.Add(this.btnBrowse);
            this.groupBox1.Controls.Add(this.txtFile);
            this.groupBox1.Controls.Add(this.lblFile);
            this.groupBox1.Location = new System.Drawing.Point(5, 5);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(580, 520);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "UC1617S 图片取模";
            // 
            // lblSizeInfo
            // 
            this.lblSizeInfo.AutoSize = true;
            this.lblSizeInfo.ForeColor = System.Drawing.Color.Gray;
            this.lblSizeInfo.Location = new System.Drawing.Point(8, 500);
            this.lblSizeInfo.Name = "lblSizeInfo";
            this.lblSizeInfo.Size = new System.Drawing.Size(0, 12);
            this.lblSizeInfo.TabIndex = 22;
            // 
            // btnCopy
            // 
            this.btnCopy.Location = new System.Drawing.Point(470, 462);
            this.btnCopy.Name = "btnCopy";
            this.btnCopy.Size = new System.Drawing.Size(100, 25);
            this.btnCopy.TabIndex = 21;
            this.btnCopy.Text = "复制到剪贴板";
            this.btnCopy.UseVisualStyleBackColor = true;
            this.btnCopy.Click += new System.EventHandler(this.btnCopy_Click);
            // 
            // btnExportC
            // 
            this.btnExportC.Location = new System.Drawing.Point(360, 462);
            this.btnExportC.Name = "btnExportC";
            this.btnExportC.Size = new System.Drawing.Size(100, 25);
            this.btnExportC.TabIndex = 20;
            this.btnExportC.Text = "导出 .c";
            this.btnExportC.UseVisualStyleBackColor = true;
            this.btnExportC.Click += new System.EventHandler(this.btnExportC_Click);
            // 
            // btnExportH
            // 
            this.btnExportH.Location = new System.Drawing.Point(250, 462);
            this.btnExportH.Name = "btnExportH";
            this.btnExportH.Size = new System.Drawing.Size(100, 25);
            this.btnExportH.TabIndex = 19;
            this.btnExportH.Text = "导出 .h";
            this.btnExportH.UseVisualStyleBackColor = true;
            this.btnExportH.Click += new System.EventHandler(this.btnExportH_Click);
            // 
            // btnPreview
            // 
            this.btnPreview.Location = new System.Drawing.Point(130, 462);
            this.btnPreview.Name = "btnPreview";
            this.btnPreview.Size = new System.Drawing.Size(110, 25);
            this.btnPreview.TabIndex = 18;
            this.btnPreview.Text = "预览数据";
            this.btnPreview.UseVisualStyleBackColor = true;
            this.btnPreview.Click += new System.EventHandler(this.btnPreview_Click);
            // 
            // btnConvert
            // 
            this.btnConvert.Location = new System.Drawing.Point(10, 462);
            this.btnConvert.Name = "btnConvert";
            this.btnConvert.Size = new System.Drawing.Size(110, 25);
            this.btnConvert.TabIndex = 17;
            this.btnConvert.Text = "转换";
            this.btnConvert.UseVisualStyleBackColor = true;
            this.btnConvert.Click += new System.EventHandler(this.btnConvert_Click);
            // 
            // txtPreview
            // 
            this.txtPreview.Font = new System.Drawing.Font("Consolas", 9F);
            this.txtPreview.Location = new System.Drawing.Point(10, 271);
            this.txtPreview.Multiline = true;
            this.txtPreview.Name = "txtPreview";
            this.txtPreview.ReadOnly = true;
            this.txtPreview.ScrollBars = System.Windows.Forms.ScrollBars.Vertical;
            this.txtPreview.Size = new System.Drawing.Size(560, 183);
            this.txtPreview.TabIndex = 16;
            // 
            // lblThInfo
            // 
            this.lblThInfo.AutoSize = true;
            this.lblThInfo.ForeColor = System.Drawing.Color.Gray;
            this.lblThInfo.Location = new System.Drawing.Point(162, 251);
            this.lblThInfo.Name = "lblThInfo";
            this.lblThInfo.Size = new System.Drawing.Size(317, 12);
            this.lblThInfo.TabIndex = 15;
            this.lblThInfo.Text = "白  ←  浅灰  ←  深灰  ←  黑    (拖动滑块实时预览)";
            // 
            // lblTh3Val
            // 
            this.lblTh3Val.AutoSize = true;
            this.lblTh3Val.Location = new System.Drawing.Point(540, 231);
            this.lblTh3Val.Name = "lblTh3Val";
            this.lblTh3Val.Size = new System.Drawing.Size(23, 12);
            this.lblTh3Val.TabIndex = 14;
            this.lblTh3Val.Text = "192";
            // 
            // lblTh2Val
            // 
            this.lblTh2Val.AutoSize = true;
            this.lblTh2Val.Location = new System.Drawing.Point(540, 208);
            this.lblTh2Val.Name = "lblTh2Val";
            this.lblTh2Val.Size = new System.Drawing.Size(23, 12);
            this.lblTh2Val.TabIndex = 13;
            this.lblTh2Val.Text = "128";
            // 
            // lblTh1Val
            // 
            this.lblTh1Val.AutoSize = true;
            this.lblTh1Val.Location = new System.Drawing.Point(540, 185);
            this.lblTh1Val.Name = "lblTh1Val";
            this.lblTh1Val.Size = new System.Drawing.Size(17, 12);
            this.lblTh1Val.TabIndex = 12;
            this.lblTh1Val.Text = "64";
            // 
            // trackTh3
            // 
            this.trackTh3.LargeChange = 16;
            this.trackTh3.Location = new System.Drawing.Point(65, 226);
            this.trackTh3.Maximum = 255;
            this.trackTh3.Name = "trackTh3";
            this.trackTh3.Size = new System.Drawing.Size(470, 45);
            this.trackTh3.TabIndex = 11;
            this.trackTh3.TickFrequency = 8;
            this.trackTh3.Value = 192;
            this.trackTh3.Scroll += new System.EventHandler(this.trackTh_Scroll);
            // 
            // trackTh2
            // 
            this.trackTh2.LargeChange = 16;
            this.trackTh2.Location = new System.Drawing.Point(65, 203);
            this.trackTh2.Maximum = 255;
            this.trackTh2.Name = "trackTh2";
            this.trackTh2.Size = new System.Drawing.Size(470, 45);
            this.trackTh2.TabIndex = 10;
            this.trackTh2.TickFrequency = 8;
            this.trackTh2.Value = 128;
            this.trackTh2.Scroll += new System.EventHandler(this.trackTh_Scroll);
            // 
            // trackTh1
            // 
            this.trackTh1.LargeChange = 16;
            this.trackTh1.Location = new System.Drawing.Point(65, 180);
            this.trackTh1.Maximum = 255;
            this.trackTh1.Name = "trackTh1";
            this.trackTh1.Size = new System.Drawing.Size(470, 45);
            this.trackTh1.TabIndex = 9;
            this.trackTh1.TickFrequency = 8;
            this.trackTh1.Value = 64;
            this.trackTh1.Scroll += new System.EventHandler(this.trackTh_Scroll);
            // 
            // lblTh3
            // 
            this.lblTh3.AutoSize = true;
            this.lblTh3.Location = new System.Drawing.Point(8, 231);
            this.lblTh3.Name = "lblTh3";
            this.lblTh3.Size = new System.Drawing.Size(53, 12);
            this.lblTh3.TabIndex = 8;
            this.lblTh3.Text = "深灰/黑:";
            // 
            // lblTh2
            // 
            this.lblTh2.AutoSize = true;
            this.lblTh2.Location = new System.Drawing.Point(8, 208);
            this.lblTh2.Name = "lblTh2";
            this.lblTh2.Size = new System.Drawing.Size(59, 12);
            this.lblTh2.TabIndex = 7;
            this.lblTh2.Text = "浅灰/深灰";
            // 
            // lblTh1
            // 
            this.lblTh1.AutoSize = true;
            this.lblTh1.Location = new System.Drawing.Point(8, 185);
            this.lblTh1.Name = "lblTh1";
            this.lblTh1.Size = new System.Drawing.Size(59, 12);
            this.lblTh1.TabIndex = 6;
            this.lblTh1.Text = "白 /浅灰:";
            // 
            // picOrig
            // 
            this.picOrig.BackColor = System.Drawing.Color.White;
            this.picOrig.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.picOrig.Location = new System.Drawing.Point(310, 78);
            this.picOrig.Name = "picOrig";
            this.picOrig.Size = new System.Drawing.Size(260, 95);
            this.picOrig.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.picOrig.TabIndex = 5;
            this.picOrig.TabStop = false;
            // 
            // picPreview
            // 
            this.picPreview.BackColor = System.Drawing.Color.White;
            this.picPreview.BorderStyle = System.Windows.Forms.BorderStyle.FixedSingle;
            this.picPreview.Location = new System.Drawing.Point(10, 78);
            this.picPreview.Name = "picPreview";
            this.picPreview.Size = new System.Drawing.Size(260, 95);
            this.picPreview.SizeMode = System.Windows.Forms.PictureBoxSizeMode.Zoom;
            this.picPreview.TabIndex = 4;
            this.picPreview.TabStop = false;
            // 
            // lblPreview
            // 
            this.lblPreview.AutoSize = true;
            this.lblPreview.Location = new System.Drawing.Point(10, 63);
            this.lblPreview.Name = "lblPreview";
            this.lblPreview.Size = new System.Drawing.Size(59, 12);
            this.lblPreview.TabIndex = 3;
            this.lblPreview.Text = "转换预览:";
            // 
            // lblOrig
            // 
            this.lblOrig.AutoSize = true;
            this.lblOrig.Location = new System.Drawing.Point(310, 63);
            this.lblOrig.Name = "lblOrig";
            this.lblOrig.Size = new System.Drawing.Size(47, 12);
            this.lblOrig.TabIndex = 2;
            this.lblOrig.Text = "原  图:";
            // 
            // txtName
            // 
            this.txtName.Location = new System.Drawing.Point(430, 33);
            this.txtName.Name = "txtName";
            this.txtName.Size = new System.Drawing.Size(140, 21);
            this.txtName.TabIndex = 1;
            this.txtName.Text = "image_data";
            // 
            // lblName
            // 
            this.lblName.AutoSize = true;
            this.lblName.Location = new System.Drawing.Point(375, 37);
            this.lblName.Name = "lblName";
            this.lblName.Size = new System.Drawing.Size(47, 12);
            this.lblName.TabIndex = 0;
            this.lblName.Text = "数组名:";
            // 
            // txtH
            // 
            this.txtH.Location = new System.Drawing.Point(332, 33);
            this.txtH.Name = "txtH";
            this.txtH.Size = new System.Drawing.Size(28, 21);
            this.txtH.TabIndex = 1;
            this.txtH.Text = "96";
            // 
            // lblH
            // 
            this.lblH.AutoSize = true;
            this.lblH.Location = new System.Drawing.Point(294, 37);
            this.lblH.Name = "lblH";
            this.lblH.Size = new System.Drawing.Size(35, 12);
            this.lblH.TabIndex = 0;
            this.lblH.Text = "高度:";
            // 
            // txtW
            // 
            this.txtW.Location = new System.Drawing.Point(263, 33);
            this.txtW.Name = "txtW";
            this.txtW.Size = new System.Drawing.Size(29, 21);
            this.txtW.TabIndex = 1;
            this.txtW.Text = "128";
            // 
            // lblW
            // 
            this.lblW.AutoSize = true;
            this.lblW.Location = new System.Drawing.Point(225, 37);
            this.lblW.Name = "lblW";
            this.lblW.Size = new System.Drawing.Size(35, 12);
            this.lblW.TabIndex = 0;
            this.lblW.Text = "宽度:";
            // 
            // btnBrowse
            // 
            this.btnBrowse.Location = new System.Drawing.Point(179, 32);
            this.btnBrowse.Name = "btnBrowse";
            this.btnBrowse.Size = new System.Drawing.Size(35, 23);
            this.btnBrowse.TabIndex = 1;
            this.btnBrowse.Text = "...";
            this.btnBrowse.UseVisualStyleBackColor = true;
            this.btnBrowse.Click += new System.EventHandler(this.btnBrowse_Click);
            // 
            // txtFile
            // 
            this.txtFile.Location = new System.Drawing.Point(10, 33);
            this.txtFile.Name = "txtFile";
            this.txtFile.ReadOnly = true;
            this.txtFile.Size = new System.Drawing.Size(163, 21);
            this.txtFile.TabIndex = 1;
            // 
            // lblFile
            // 
            this.lblFile.AutoSize = true;
            this.lblFile.Location = new System.Drawing.Point(10, 18);
            this.lblFile.Name = "lblFile";
            this.lblFile.Size = new System.Drawing.Size(59, 12);
            this.lblFile.TabIndex = 0;
            this.lblFile.Text = "图片文件:";
            // 
            // Form1
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(590, 530);
            this.Controls.Add(this.groupBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "Form1";
            this.Text = "UC1617S 图片取模工具";
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.trackTh3)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackTh2)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackTh1)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.picOrig)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.picPreview)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Label lblFile;
        private System.Windows.Forms.TextBox txtFile;
        private System.Windows.Forms.Button btnBrowse;
        private System.Windows.Forms.Label lblW;
        private System.Windows.Forms.TextBox txtW;
        private System.Windows.Forms.Label lblH;
        private System.Windows.Forms.TextBox txtH;
        private System.Windows.Forms.Label lblName;
        private System.Windows.Forms.TextBox txtName;
        private System.Windows.Forms.Label lblOrig;
        private System.Windows.Forms.Label lblPreview;
        private System.Windows.Forms.PictureBox picPreview;
        private System.Windows.Forms.PictureBox picOrig;
        private System.Windows.Forms.Label lblTh1;
        private System.Windows.Forms.Label lblTh2;
        private System.Windows.Forms.Label lblTh3;
        private System.Windows.Forms.TrackBar trackTh1;
        private System.Windows.Forms.TrackBar trackTh2;
        private System.Windows.Forms.TrackBar trackTh3;
        private System.Windows.Forms.Label lblTh1Val;
        private System.Windows.Forms.Label lblTh2Val;
        private System.Windows.Forms.Label lblTh3Val;
        private System.Windows.Forms.Label lblThInfo;
        private System.Windows.Forms.TextBox txtPreview;
        private System.Windows.Forms.Button btnConvert;
        private System.Windows.Forms.Button btnPreview;
        private System.Windows.Forms.Button btnExportH;
        private System.Windows.Forms.Button btnExportC;
        private System.Windows.Forms.Button btnCopy;
        private System.Windows.Forms.Label lblSizeInfo;
    }
}
