using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;


namespace PlanEditor
{
    /// <summary>
    /// Interaction logic for MapBuilder.xaml
    /// </summary>
    public partial class MapBuilderWindow : Window
    {
        MapResultWindow mapResultWindow;
        MapBuilder mapBuilder;
        Image navImage;

        public MapBuilderWindow()
        {
            mapResultWindow = new MapResultWindow();
            //mapResultWindow.Hide();
            //mapResultWindow.Show();
            InitializeComponent();

            this.Closing += new System.ComponentModel.CancelEventHandler(MapBuilderWindow_Closing);
            
            mapBuilder = new MapBuilder(mapCanvas);
            navImage = new Image();
            this.mapCanvas.Children.Add(navImage);
           
        }

        void MapBuilderWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            this.Visibility = Visibility.Hidden;
            e.Cancel = true;
        }

       

       



        private void setImageSource(BitmapSource img)
        {
            navImage.Source = img;
            this.mapCanvas.Width = navImage.Width;
            this.mapCanvas.Height = navImage.Height;
        }
        public void setNavMap(NavMap nav)
        {
            this.mapBuilder.NavMap = nav;
            this.DataContext = mapBuilder.NavMap;
            
            setImageSource(nav.getWholeImage());
            
            
        }

        public void BuildMap()
        {

            //Then builds it
            mapBuilder.NavMap.build();

            //Shows the next window
            mapResultWindow.setNavMapAndShow(mapBuilder.NavMap);

           
        }

        private void buildBtn_Click(object sender, RoutedEventArgs e)
        {
            BuildMap();
        }

        private void buildAndExportBtn_Click(object sender, RoutedEventArgs e)
        {

            Microsoft.Win32.SaveFileDialog fileSave = new Microsoft.Win32.SaveFileDialog();
            fileSave.FileName = "untitlemap";
            fileSave.DefaultExt = ".xml";
            fileSave.Filter = "Xml documents (.xml)|*.xml";

            Nullable<bool> result = fileSave.ShowDialog();
            if(result== true)
            {
                this.mapBuilder.NavMap.buildAndExportThreaded(fileSave.FileName);
            }
        }

    


        

        
        
    }



}
