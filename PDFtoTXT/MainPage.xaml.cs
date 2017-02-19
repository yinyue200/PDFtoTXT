using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Globalization;
using Windows.Media.Ocr;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// https://go.microsoft.com/fwlink/?LinkId=402352&clcid=0x804 上介绍了“空白页”项模板

namespace PDFtoTXT
{
    /// <summary>
    /// 可用于自身或导航至 Frame 内部的空白页。
    /// </summary>
    public sealed partial class MainPage : Page
    {
        Windows.Storage.IStorageFile pdffile;
        Windows.Storage.IStorageFile txtfile;
        Windows.ApplicationModel.Resources.ResourceLoader loader = Windows.ApplicationModel.Resources.ResourceLoader.GetForCurrentView();
        Language lang;
        public MainPage()
        {
            this.InitializeComponent();
            lans.ItemsSource = OcrEngine.AvailableRecognizerLanguages;
            lans.SelectedValue = "Auto";
        }

        private async void Button_Click(object sender, RoutedEventArgs e)
        {
            Windows.Storage.Pickers.FileOpenPicker fop = new Windows.Storage.Pickers.FileOpenPicker();
            fop.FileTypeFilter.Add(".pdf");
            pdffile = await fop.PickSingleFileAsync();
        }

        private async void Button_Click_1(object sender, RoutedEventArgs e)
        {
            var fop = new Windows.Storage.Pickers.FileSavePicker();
            fop.FileTypeChoices.Add(".txt",new List<string> { ".txt" });
            txtfile = await fop.PickSaveFileAsync();
        }

        private async void Button_Click_2(object sender, RoutedEventArgs e)
        {
            if(txtfile==null||pdffile==null)
            {
                await new Windows.UI.Popups.MessageDialog(loader.GetString("error_pleasechoosefilefirst")).ShowAsync();
                return;
            }
            ct.IsEnabled = false;
            cp.IsEnabled = false;
            start.IsEnabled = false;
            try
            {
                Windows.Data.Pdf.PdfDocument pd = await Windows.Data.Pdf.PdfDocument.LoadFromFileAsync(pdffile);
                if (pd.IsPasswordProtected)
                {
                    await new Windows.UI.Popups.MessageDialog(loader.GetString("error_pdfwithpassword")).ShowAsync();
                    return;
                }
                pro.Value = 0;
                pro.Maximum = pd.PageCount;
                var ocre = lang == null ? OcrEngine.TryCreateFromUserProfileLanguages() : OcrEngine.TryCreateFromLanguage(lang);
                if(ocre==null&&lang==null)
                {
                    await new Windows.UI.Popups.MessageDialog(loader.GetString("error_notsupport")).ShowAsync();
                    return;
                }               
                using (Stream txtstr = await txtfile.OpenStreamForWriteAsync())
                {
                    using (StreamWriter sw = new StreamWriter(txtstr))
                    {
                        for (uint i = 0; i < pd.PageCount; i++)
                        {
                            using (var page = pd.GetPage(i))
                            {
                                await page.PreparePageAsync();
                                using (var str = new Windows.Storage.Streams.InMemoryRandomAccessStream())
                                {
                                    await page.RenderToStreamAsync(str);
                                    var decoder = await Windows.Graphics.Imaging.BitmapDecoder.CreateAsync(str);
                                    var re = await ocre.RecognizeAsync(await decoder.GetSoftwareBitmapAsync());
                                    sw.WriteLine(re.Text);
                                }
                            }
                            pro.Value++;
                        }
                    }
                }

            }
            catch
            {
                await new Windows.UI.Popups.MessageDialog(loader.GetString("error")).ShowAsync();
            }
            finally
            {
                ct.IsEnabled = true;
                cp.IsEnabled = true;
                start.IsEnabled = true;
                txtfile = null;
                pdffile = null;
            }
        }

        private void lans_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            lang = lans.SelectedValue as Language;
        }
    }
}
