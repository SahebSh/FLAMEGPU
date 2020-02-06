using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls.Primitives;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Win32;

namespace PlanEditor
{


    
    /// <summary>
    /// Interaction logic for Window1.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
       

        private MapBuilderWindow mapBuilderDialog;
        private OpenFileDialog imageFileChooserDialog;

        private PlanEditManager planManager;

        private ToggleButtonGroup toolsToggleGroup;

        private OpenFileDialog loadDataDialog = new OpenFileDialog();
        private SaveFileDialog saveDataDialog = new SaveFileDialog();

        public MainWindow()
        {

            try
            {

                InitializeComponent();

                InitLogic();
                //Hide the dialog
                mapBuilderDialog = new MapBuilderWindow();
                // mapBuilderDialog.Hide();

                string datafileFilter = "Planeditor (.xped)|*.xped";
                loadDataDialog.Filter = datafileFilter;
                saveDataDialog.Filter = datafileFilter;
                saveDataDialog.FileName = "untitled";

            }
            catch (Exception e)
            {
                Console.WriteLine(e);
            }
        }

        
        

        private void InitLogic()
        {
            //Init Dialog
            imageFileChooserDialog = new OpenFileDialog();

            //Create a plan edit manager
            planManager = new PlanEditManager(editorCanvas);

            //Makes the scroll viwer scroll when the mouse gets close to the
            //edge
            editorScroll.MouseMove += new MouseEventHandler(editorCanvas_MouseMove);

           // wallToolBtn.Checked(
            try
            {
                toolsToggleGroup = new ToggleButtonGroup();
                areaToolBtn.Checked += new RoutedEventHandler(areaToolBtn_Checked);
                wallToolBtn.Checked += new RoutedEventHandler(wallToolBtn_Checked);
                doorToolBtn.Checked += new RoutedEventHandler(doorToolBtn_Checked);
                toolsToggleGroup.Add(PlanEditManager.CurrentTool.AreaRect.ToString(), areaToolBtn);
                toolsToggleGroup.Add(PlanEditManager.CurrentTool.Wall.ToString(), wallToolBtn);
                toolsToggleGroup.Add(PlanEditManager.CurrentTool.Exit.ToString(), doorToolBtn);
            }
            catch (Exception e)
            {
                Console.WriteLine(e.ToString());
            }

        }

        public void newFile()
        {
            this.editorCanvas.Children.Clear();
            planManager = new PlanEditManager(editorCanvas);
        }

        void doorToolBtn_Checked(object sender, RoutedEventArgs e)
        {
            
            planManager.setCurrentTool(PlanEditManager.CurrentTool.Exit);
        }

        void wallToolBtn_Checked(object sender, RoutedEventArgs e)
        {
            planManager.setCurrentTool(PlanEditManager.CurrentTool.Wall);
        }

        void areaToolBtn_Checked(object sender, RoutedEventArgs e)
        {
            planManager.setCurrentTool(PlanEditManager.CurrentTool.AreaRect);
        }

   

        void editorCanvas_MouseMove(object sender, MouseEventArgs e)
        {

            planManager.mouseMoveActions(sender, e);
           
        }

        private void Window_Closed(object sender, EventArgs e)
        {
            Application.Current.Shutdown();
        }

        private void MenuShutdown(object sender, RoutedEventArgs e)
        {
            Application.Current.Shutdown();
        }

    
        
        private void zoomInBtn_Click(object sender, RoutedEventArgs e)
        {
            planManager.zoom(2);
        }

       
        private void zoomOutBtn_Click(object sender, RoutedEventArgs e)
        {
            planManager.zoom(0.5);
        }

  

       

        private void loadImgBtn_Click(object sender, RoutedEventArgs e)
        {
            Nullable<bool> result = this.imageFileChooserDialog.ShowDialog();
            if (result == true)
            {
                planManager.loadBackgroundImage(this.imageFileChooserDialog.FileName);
            }
        }

        private void buildBtn_Click(object sender, RoutedEventArgs e)
        {
            int width = -1;
            int height = -1;
            try
            {
                if(buildWidthTxt.Text.Length > 0)
                width = System.Convert.ToInt32(buildWidthTxt.Text);
            }
            catch (Exception exp)
            {
                
               
            }
            try
            {
                if(buildHeightTxt.Text.Length > 0)
                height = System.Convert.ToInt32(buildHeightTxt.Text);
            }
            catch (Exception exp)
            {
                
                
            }
            if ( width > 0 || height > 0)
            {

                mapBuilderDialog.setNavMap(planManager.buildMap(width, height));
                mapBuilderDialog.Show();
            }
            else
            {
                MessageBox.Show("Set the size of the area first! (Can set either width or height or both)");
            }
        }

        private void loadDataMenu_Click(object sender, RoutedEventArgs e)
        {
             Nullable<bool> result = this.loadDataDialog.ShowDialog(this);
            if(result == true)
            {
                planManager.load(loadDataDialog.FileName);
            }
        }

        private void saveDataMenu_Click(object sender, RoutedEventArgs e)
        {
            Nullable<bool> result = this.saveDataDialog.ShowDialog(this);
            if(result == true)
            {
                planManager.save(saveDataDialog.FileName);
            }
        }

        private void newDataMenu_Click(object sender, RoutedEventArgs e)
        {
            this.newFile();
        }


        private bool ctrlBtnDown = false;
        private void windowGrid_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.LeftCtrl)
            {
                ctrlBtnDown = false;
            }
        }

        private void windowGrid_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.Key == Key.LeftCtrl)
            {
                ctrlBtnDown = true;
            }

            if (ctrlBtnDown && e.Key == Key.Z)
            {
                this.planManager.undo();
            }
        }


        

        private void undoMenu_Click(object sender, RoutedEventArgs e)
        {


            this.planManager.undo();
        }



        private void altRefBtn_Click(object sender, RoutedEventArgs e)
        {
            this.planManager.alternateReferences();
        }

        OpenFileDialog heightImageFileChooserDialog = new OpenFileDialog();
        private void loadHmapBtn_Click(object sender, RoutedEventArgs e)
        {
            Nullable<bool> result = this.heightImageFileChooserDialog.ShowDialog(this);
            if (result == true)
            {
                planManager.loadHeightmapImage(this.heightImageFileChooserDialog.FileName);
            }
        }

        private void matchImageAreaBtn_Click(object sender, RoutedEventArgs e)
        {
            this.planManager.matchAreaToImage();
        }

        private void drawCollisionAgentBtn_Click(object sender, RoutedEventArgs e)
        {
            this.planManager.setCurrentTool(PlanEditManager.CurrentTool.CollisionAgent);
        }

        private void obstacleBtn_Click(object sender, RoutedEventArgs e)
        {
            this.planManager.setCurrentTool(PlanEditManager.CurrentTool.Obstacle);
        }

        private void shopBtn_Click(object sender, RoutedEventArgs e)
        {
            this.planManager.setCurrentTool(PlanEditManager.CurrentTool.Shop);
        }

       
       
        
    }

    public class ToggleButtonGroup
    {
        Dictionary<string, ToggleButton> btnDict;

        public ToggleButtonGroup()
        {
            btnDict = new Dictionary<string, ToggleButton>();
        }

        public void Add(string name, ToggleButton btn)
        {
            btnDict.Add(name, btn);
            btn.Click += new RoutedEventHandler(btn_Click);
        }

        public bool ToggleButton(string name)
        {
            if (btnDict.ContainsKey(name))
            {
                foreach (string key in btnDict.Keys)
                {
                    if (key.CompareTo(name) == 0)
                    {
                        btnDict[key].IsChecked = true;
                    }
                    else
                    {
                        btnDict[key].IsChecked = false;
                    }
                }

                return true;
            }

            return false;
        }

        void btn_Click(object sender, RoutedEventArgs e)
        {
            //Turn all the others off
            foreach (ToggleButton t in btnDict.Values)
            {
                if (t != sender)
                {
                    t.IsChecked = false;
                }
            }
        }




    }

    public class DatSize
    {
        public int Width { get; set; }
        public int Height { get; set; }
        public DatSize() { }
    }

}
