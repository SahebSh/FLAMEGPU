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
using System.Windows.Media.Media3D;
using System.Windows.Media.Media3D.Converters;

namespace PlanEditor
{
    /// <summary>
    /// Interaction logic for MapResultWindow.xaml
    /// </summary>
    public partial class MapResultWindow : Window
    {
        
        private Microsoft.Win32.SaveFileDialog saveDialog = new Microsoft.Win32.SaveFileDialog();
        private NavMap navMap;
        Image navImage;

        private bool middleDown;
        private bool mDown;
        private Point middleLastPos;
        private Point mLastPos;
        private MeshGeometry3D arrowMesh = new MeshGeometry3D();
        private MeshGeometry3D squareMesh = new MeshGeometry3D();
        private DiffuseMaterial redMaterial = new DiffuseMaterial(Brushes.Red);

        public MapResultWindow()
        {
            InitializeComponent();
            layershowCombo.IsEnabled = false;
            layershowCombo.SelectionChanged += new SelectionChangedEventHandler(layershowCombo_SelectionChanged);
            this.Closing += new System.ComponentModel.CancelEventHandler(MapResultWindow_Closing);
            this.navImage = new Image();
            

            saveDialog.FileName = "untitledmap";
            saveDialog.DefaultExt = ".xml";
            saveDialog.Filter = "Xml documents (.xml)|*.xml";

            

            //Builds the arrow for visualization
            init3D();

            
            

        }

        private void init3D()
        {
            // Define 3D mesh object
            //arrowMesh = new MeshGeometry3D();
            //arrowMesh.Positions.Add(new Point3D(-0.5, -0.25, 0));
            ////arrowMesh.Normals.Add(new Vector3D(0, 0, 1));
            //arrowMesh.Positions.Add(new Point3D(1, 0, 0));
            ////arrowMesh.Normals.Add(new Vector3D(0, 0, 1));
            //arrowMesh.Positions.Add(new Point3D(-0.5, 0.25, 0));
            ////arrowMesh.Normals.Add(new Vector3D(0, 0, 1));

            arrowMesh.Positions.Add(new Point3D(-0.25, -0.4, 0));
            //arrowMesh.Normals.Add(new Vector3D(0, 0, 1));
            arrowMesh.Positions.Add(new Point3D(0.25, -0.4, 0));
            //arrowMesh.Normals.Add(new Vector3D(0, 0, 1));
            arrowMesh.Positions.Add(new Point3D(0, 0.4, 0));
            //arrowMesh.Normals.Add(new Vector3D(0, 0, 1));

          
            
            // Front face
            arrowMesh.TriangleIndices.Add(0);
            arrowMesh.TriangleIndices.Add(1);
            arrowMesh.TriangleIndices.Add(2);
            arrowMesh.TriangleIndices.Add(2);
            arrowMesh.TriangleIndices.Add(1);
            arrowMesh.TriangleIndices.Add(0);



            squareMesh.Positions.Add(new Point3D(-0.4, -0.4, 0));
            //arrowMesh.Normals.Add(new Vector3D(0, 0, 1));
            squareMesh.Positions.Add(new Point3D(0.4, -0.4, 0));
            //arrowMesh.Normals.Add(new Vector3D(0, 0, 1));
            squareMesh.Positions.Add(new Point3D(0.4, 0.4, 0));
            //arrowMesh.Normals.Add(new Vector3D(0, 0, 1));
            squareMesh.Positions.Add(new Point3D(-0.4, 0.4, 0));



            // Front face
            squareMesh.TriangleIndices.Add(0);
            squareMesh.TriangleIndices.Add(1);
            squareMesh.TriangleIndices.Add(2);
            squareMesh.TriangleIndices.Add(0);
            squareMesh.TriangleIndices.Add(2);
            squareMesh.TriangleIndices.Add(3);

            


            

            //Sets up transformation of the group
            Transform3DGroup gr = new Transform3DGroup();
            model.Transform = gr;
            

        }

        private void Grid_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            
            camera.Position = new Point3D(camera.Position.X, camera.Position.Y, camera.Position.Z - e.Delta/10D );
        }

        //private void Button_Click(object sender, RoutedEventArgs e)
        //{
        //    camera.Position = new Point3D(camera.Position.X, camera.Position.Y, 5);
        //    mGeometry.Transform = new Transform3DGroup();
        //}

        private void Grid_MouseMove(object sender, MouseEventArgs e)
        {
            if (mDown)
            {
                
                
                //Point pos = Mouse.GetPosition(viewport);
                //Point actualPos = new Point(pos.X - viewport.ActualWidth / 2, viewport.ActualHeight / 2 - pos.Y);
                //double dx = actualPos.X - mLastPos.X, dy = actualPos.Y - mLastPos.Y;

                //double mouseAngle = 0;
                //if (dx != 0 && dy != 0)
                //{
                //    mouseAngle = Math.Asin(Math.Abs(dy) / Math.Sqrt(Math.Pow(dx, 2) + Math.Pow(dy, 2)));
                //    if (dx < 0 && dy > 0) mouseAngle += Math.PI / 2;
                //    else if (dx < 0 && dy < 0) mouseAngle += Math.PI;
                //    else if (dx > 0 && dy < 0) mouseAngle += Math.PI * 1.5;
                //}
                //else if (dx == 0 && dy != 0) mouseAngle = Math.Sign(dy) > 0 ? Math.PI / 2 : Math.PI * 1.5;
                //else if (dx != 0 && dy == 0) mouseAngle = Math.Sign(dx) > 0 ? 0 : Math.PI;

                //double axisAngle = mouseAngle + Math.PI / 2;

                //Vector3D axis = new Vector3D(Math.Cos(axisAngle) * 4, Math.Sin(axisAngle) * 4, 0);

                //double rotation = 0.01 * Math.Sqrt(Math.Pow(dx, 2) + Math.Pow(dy, 2));

                //Transform3DGroup transformGroup = model.Transform as Transform3DGroup;
                //QuaternionRotation3D r = new QuaternionRotation3D(new Quaternion(axis, rotation * 180 / Math.PI));
                //transformGroup.Children.Add(new RotateTransform3D(r));

                //mLastPos = actualPos;
            }

            if (middleDown)
            {
                Point pos = Mouse.GetPosition(viewportGrid);
                double xmove = pos.X - middleLastPos.X;
                double ymove = pos.Y - middleLastPos.Y;
                double slowfactor = 0.1;
                camera.Position = new Point3D(camera.Position.X - xmove*slowfactor, camera.Position.Y + ymove*slowfactor, camera.Position.Z);


                middleLastPos = new Point(pos.X, pos.Y);
            }
        }

        private void Grid_MouseDown(object sender, MouseButtonEventArgs e)
        {

            if (e.LeftButton == MouseButtonState.Pressed)
            {
                mDown = true;
                Point pos = Mouse.GetPosition(viewport);
                mLastPos = new Point(pos.X - viewport.ActualWidth / 2, viewport.ActualHeight / 2 - pos.Y);
            }

            if (e.MiddleButton == MouseButtonState.Pressed)
            {
                middleDown = true;
                Point pos = Mouse.GetPosition(viewportGrid);
                middleLastPos = new Point(pos.X, pos.Y);
                
            }
        }

        private void Grid_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Released)
            {
                mDown = false;
            }

            if (e.MiddleButton == MouseButtonState.Released)
            {
                middleDown = false;
            }
            
        }

        void MapResultWindow_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            e.Cancel = true;
            this.Visibility = Visibility.Hidden;
        }

        private void setImageSource(BitmapSource img)
        {
            navImage.Source = img;
            
            
        }

        private void showDifferentLayer()
        {
            Vector3D zRotVector = new Vector3D(0, 0, 1);
            string strtype = "rndstring";
            int intType = 2;


            Model3DGroup g = new Model3DGroup();
            ModelVisual3D mv = new ModelVisual3D();
            g.Children.Add(new AmbientLight(Color.FromRgb(1, 1, 1)));
            g.Children.Add(new DirectionalLight(Color.FromRgb(1, 1, 1), new Vector3D(-5, -5, -7)));

            //GridPoint[,] debugGrid = navMap.getTestSurround();

            DiffuseMaterial walkableMaterial = new DiffuseMaterial(new SolidColorBrush(Color.FromRgb(220,69,69)));
            DiffuseMaterial wallMaterial = new DiffuseMaterial(new SolidColorBrush(Colors.Black));
            DiffuseMaterial exitMaterial = new DiffuseMaterial(new SolidColorBrush(Colors.Red));
            DiffuseMaterial nullMaterial = new DiffuseMaterial(new SolidColorBrush(Colors.White));
            DiffuseMaterial collisionMaterial = new DiffuseMaterial(new SolidColorBrush(Colors.Blue));

            if (layershowCombo.Items.Count > 0)
            {
                //group.Children.Clear();
                object selected = layershowCombo.SelectedValue;
                if (selected.GetType() == strtype.GetType())
                {

                    //Show walls
                  
                    for (int x = 0; x < navMap.Width; x++)
                    {
                        for (int y = 0; y < navMap.Height; y++)
                        {
                            DiffuseMaterial diff;
                            MeshGeometry3D mesh;

                            
                            Transform3DGroup transform = new Transform3DGroup();
                            double angle = navMap.wallGrid[x, y].WallForceAngle * (180.0 / Math.PI);
                            if (navMap.wallGrid[x, y].Type == GridPoint.PointType.Walkable)
                            {

                                transform.Children.Add(new RotateTransform3D(new AxisAngleRotation3D(zRotVector, -angle)));
                                if (navMap.wallGrid[x, y].WallX == 0 && navMap.wallGrid[x, y].WallY == 0)
                                {
                                    diff = nullMaterial;
                                }
                                else
                                {
                                    diff = collisionMaterial;
                                }
                                
                                mesh = arrowMesh;

                            }
                            else
                            {
                                
                                diff = wallMaterial;
                                mesh = squareMesh;
                                //transform.Children.Add(new RotateTransform3D(new AxisAngleRotation3D(zRotVector, -angle)));

                            }
                            
                            GeometryModel3D geom = new GeometryModel3D(mesh, diff);
                            transform.Children.Add(new TranslateTransform3D() { OffsetX = x, OffsetY = y });
                            geom.Transform = transform;
                            g.Children.Add(geom);


                        }
                    }





                }
                else if (selected.GetType() == intType.GetType())
                {
                    //Show int
                    for (int x = 0; x < navMap.Width; x++)
                    {
                        for (int y = 0; y < navMap.Height; y++)
                        {
                            //Defaults to a wall
                            DiffuseMaterial diff = wallMaterial;
                            MeshGeometry3D mesh = squareMesh ;
                            GridPoint gPoint = navMap.navGrid[(int)selected][x, y];


                            Transform3DGroup transform = new Transform3DGroup();
                            if (gPoint.Type == GridPoint.PointType.Walkable)
                            {
                                //If its a walkable
                                if (gPoint.ExitForce > 0)
                                {
                                    double angle = gPoint.ExitAngle * (180.0 / Math.PI);
                                    diff = walkableMaterial;
                                    mesh = arrowMesh;
                                    transform.Children.Add(new RotateTransform3D(new AxisAngleRotation3D(zRotVector, -angle)));
                                }
                                else
                                {
                                    
                                    double angle = gPoint.ExitAngle * (180.0 / Math.PI);
                                    diff = nullMaterial;
                                    mesh = arrowMesh;
                                    transform.Children.Add(new RotateTransform3D(new AxisAngleRotation3D(zRotVector, -angle)));
                                }
                                
                            }
                            else if (gPoint.Type == GridPoint.PointType.Exit)
                            {
                                //If its a destination
                                diff = exitMaterial;
                                mesh = squareMesh;
                            }
                            GeometryModel3D geom = new GeometryModel3D(mesh, diff);
                            transform.Children.Add(new TranslateTransform3D() { OffsetX = x, OffsetY = y });
                            geom.Transform = transform;
                            g.Children.Add(geom);
                        }
                    }
                }
            }

            mv.Content = g;
            model.Children.Clear();
            model.Children.Add(mv);
        }
        void layershowCombo_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {

            showDifferentLayer();
        }

        public void setNavMapAndShow(NavMap map)
        {
            this.navMap = map;

            layershowCombo.Items.Clear();
            layershowCombo.Items.Add("Wall force");
            for (int i = 0; i < navMap.navGrid.Count; i++)
            {
                layershowCombo.Items.Add(i);
            }
            layershowCombo.IsEnabled = true;
            layershowCombo.SelectedIndex = 0;
            this.Show();
        }

        private void exportBtn_Click(object sender, RoutedEventArgs e)
        {
            Nullable<bool> result = saveDialog.ShowDialog();
            if (result == true)
            {
                //gets the filename
                string filename = saveDialog.FileName;
                navMap.export(filename);
            }


        }

        private void mapViewport_MouseWheel(object sender, MouseWheelEventArgs e)
        {

        }

    }
}
