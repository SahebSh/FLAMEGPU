using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Drawing;
using System.Windows.Media.Media3D;
using System.IO;
using System.Threading;
namespace PlanEditor
{

    public enum ForceDegradationType { Linear, InverseSqure };

    #region Storage types


    [Serializable]
    public class Agent
    {
        public double X { get; set; }
        public double Y { get; set; }
        public double Z { get; set; }
        public double Radius { get; set; }
        public double Force { get; set; }

        public Agent getDisplace(double dx, double dy, double scale)
        {
            return new Agent() { X = (X - dx)*scale, Y = (Y - dy)*scale, Radius = Radius * scale, Force = Force };
        }
    }

    [Serializable]
    public class StringHolder
    {
        public string Value;
    }

    [Serializable]
    public class SPoint
    {
        public double X {get; set;}
        public double Y { get; set; }

        public SPoint getDisplaced(double xDisp, double yDisp, double xScale, double yScale)
        {
            return new SPoint() { X = (this.X - xDisp) * xScale, Y = (this.Y - yDisp) * yScale };
        }
    }

    [Serializable]
    public class SLine
    {
        public double x1, x2, y1, y2;

        public SLine getDisplaced(double xDisp, double yDisp, double xScale, double yScale)
        {
            return new SLine() { x1 = (this.x1 - xDisp) * xScale, x2 = (this.x2 - xDisp) * xScale, y1 = (this.y1 - yDisp) * yScale, y2 = (this.y2 - yDisp) * yScale };
        }

        public void draw(Graphics graphics, Pen pen)
        {
            graphics.DrawLine(pen, (float)x1, (float)y1, (float)x2, (float)y2);
        }
    }

    [Serializable]
    public class SPolyLine : List<SPoint>
    {
        public SPolyLine()
        {

        }

        public SPolyLine getDisplaced(double xDisp, double yDisp, double xScale, double yScale)
        {
            SPolyLine newPoly = new SPolyLine();
            foreach (SPoint p in this)
            {
                newPoly.Add(p.getDisplaced(xDisp, yDisp, xScale, yScale));
            }
            return newPoly;
        }

        public void draw(Graphics graphics, Pen pen)
        {
            PointF[] pary = new PointF[this.Count];
            for (int i = 0; i < this.Count; i++)
            {
                pary[i] = new PointF((float)this[i].X, (float)this[i].Y);
            }
            if (pary.Length > 1)
            {
                graphics.DrawLines(pen, pary);
            }
                
        }

        public void fill(Graphics graphics, Brush brush)
        {
            PointF[] pary = new PointF[this.Count];
            for (int i = 0; i < this.Count; i++)
            {
                pary[i] = new PointF((float)this[i].X, (float)this[i].Y);
            }
            if (pary.Length > 1)
            {
                graphics.FillPolygon(brush, pary);
            }

        }

    }

    [Serializable]
    public class SPolylineCollection : List<SPolyLine>
    {
        public SPolylineCollection()
        {

        }

        public SPolylineCollection getDisplace(double xDisp, double yDisp, double xScale, double yScale)
        {
            SPolylineCollection sp = new SPolylineCollection();
            foreach (SPolyLine line in this)
            {
                sp.Add(line.getDisplaced(xDisp, yDisp, xScale, yScale));
            }
            return sp;
        }

        public void draw(Graphics graphics, Pen pen)
        {
            foreach (SPolyLine line in this)
            {
                line.draw(graphics, pen);
            }
        }

        public void fill(Graphics graphics, Brush brush)
        {
            foreach (SPolyLine line in this)
            {
                line.fill(graphics, brush);
            }

        }
    }

    [Serializable]
    public class SRectangle
    {
        public double x, y, width, height;
    }

    #endregion


    /// <summary>
    /// Class for storing the built environmental maps
    /// </summary>
    public class NavMap
    {
        //Width and height
        public int Width { get; set; }
        public int Height { get; set; }

        private Dictionary<int, int> exitCounts;
        private Pen wallPen;
        private Pen exitPen;
        private Brush obstacleBrush;
        //Internal image
        private Bitmap navImage;
        private Bitmap backgroundImage;
        private Bitmap heightMapImage;

        //Grid
        public GridPoint[,] wallGrid;
        public List<GridPoint[,]> navGrid;
        public Double[,] heightGrid;

        //Wall collection
        public SPolylineCollection WallsList { get; set; }

        //Exits collection
        public SPolylineCollection ExitsList { get; set; }

        //Obstacles collection
        public SPolylineCollection ObstaclesList { get; set; }

        public List<Agent> CollisionAgents { get; set; }

        public List<Agent> ShopAgents { get; set; }
        

        public int getExitCount()
        {
            return ExitsList.Count;
        }

        public int WallForceDist { get; set; }
        public double WallForceMax { get; set; }
        public double  WallForceMin { get; set; }
        public ForceDegradationType WallForceDegradeType { get; set; }

        public int NumberOfStartingPedestrians { get; set; }
        public int ExitForceGradientDist { get; set; }
        public double ExitForceGradientMax { get; set; }
        public double ExitForceGradientMin { get; set; }
        public ForceDegradationType ExitForceDegradeType { get; set; }

        public int SearchbackPixels { get; set;}
        

        public NavMap(int width, int height)
        {
            Width = width;
            Height = height;
            WallForceDist = 2;
            WallForceMax = 1;
            WallForceMin = 0.2;
            ExitForceGradientDist = 2;
            ExitForceGradientMax = 1;
            ExitForceGradientMin = 1;
            SearchbackPixels = 15;
            NumberOfStartingPedestrians = 0;
            //Creates image for drawing
            navImage = new Bitmap(width, height);
            //Instantiates the collections
            WallsList = new SPolylineCollection();
            ExitsList = new SPolylineCollection();

            navGrid = new List<GridPoint[,]>();

        }

        public void setBackgroundImage(string filepath, System.Windows.Rect rectArea)
        {
            if (filepath != null)
            {
                backgroundImage = new Bitmap(Width, Height);
                Graphics bg = Graphics.FromImage(backgroundImage);
                //bg.DrawImage(Bitmap.FromFile(filepath), (float)-offsetX, (float)-offsetY);
                bg.DrawImage(Bitmap.FromFile(filepath), new Rectangle(0, 0, Width, Height), new Rectangle((int)rectArea.X, (int)rectArea.Y, (int)rectArea.Width, (int)rectArea.Height), GraphicsUnit.Pixel);
                bg.Flush();

            }

        }

        public void setHeightMapImage(string filepath, System.Windows.Rect rectArea)
        {
            
            if (filepath != null)
            {
                heightMapImage = new Bitmap(Width, Height);
                Graphics hg = Graphics.FromImage(heightMapImage);
                hg.DrawImage(Bitmap.FromFile(filepath), new Rectangle(0, 0, Width, Height), new Rectangle((int)rectArea.X, (int)rectArea.Y, (int)rectArea.Width, (int)rectArea.Height), GraphicsUnit.Pixel);
                hg.Flush();
            }
        }

       

        private void buildGrid(GridPoint[,] layerGrid, HashSet<GridPoint> wallPoints, HashSet<GridPoint> exitPoints, HashSet<GridPoint> obstaclePoints)
        {

            for (int x = 0; x < this.Width; x++)
            {
                for (int y = 0; y < this.Height; y++)
                {
                    GridPoint tempGridPoint;
                    Color col = navImage.GetPixel(x, y);

                    if (col.R == 0 && col.G == 0 && col.B == 0)
                    {
                        //Is a wall
                        tempGridPoint = new GridPoint(GridPoint.PointType.Wall);
                        wallPoints.Add(tempGridPoint);
                    }
                    else if (col.R == 255 && col.G == 0 && col.B == 0)
                    {
                        //Is a door
                        tempGridPoint = new GridPoint(GridPoint.PointType.Exit);
                        exitPoints.Add(tempGridPoint);
                    }
                    else if (col.R == 0 && col.G == 0 && col.B == 255)
                    {
                        //Is an obstacle
                        tempGridPoint = new GridPoint(GridPoint.PointType.Obstacle);
                        obstaclePoints.Add(tempGridPoint);
                    }

                    else
                    {
                        //Is a movable space
                        tempGridPoint = new GridPoint(GridPoint.PointType.Walkable);
                    }

                    tempGridPoint.X = x;
                    tempGridPoint.Y = this.Height -1 -y;
                    layerGrid[x, this.Height -1 - y] = tempGridPoint;
                }

            }
        }

        public double getAngle(Vector3D vect)
        {
            double angle = 0;
            //Angle from positive y
            if (vect.X == 0 && vect.Y == 0)
            {
                angle = 0;
            }
            else if (vect.X == 0 && vect.Y >= 0)
            {
                angle = 0;
            }
            else if (vect.X == 0 && vect.Y <= 0)
            {
                angle = Math.PI;
            }
            else if (vect.Y == 0 && vect.X >= 0)
            {
                angle = Math.PI / 2;
            }
            else if (vect.Y == 0 && vect.X <= 0)
            {
                angle = Math.PI * 1.5;
            }
            else if (vect.X >= 0 && vect.Y >= 0)
            {
                angle = (Math.PI/2) - Math.Abs( Math.Atan(vect.Y / vect.X));
            }
            else if (vect.X >= 0 && vect.Y <= 0)
            {
                angle = (Math.PI/2) + Math.Abs( Math.Atan(vect.Y / vect.X));
            }
            else if (vect.X <= 0 && vect.Y <= 0)
            {
                angle = (Math.PI * 1.5) - Math.Abs(Math.Atan(vect.Y / vect.X));
            }
            else if (vect.X <= 0 && vect.Y >= 0)
            {
                angle = (Math.PI * 1.5) + Math.Abs(Math.Atan(vect.Y / vect.X));
            }
            
            return angle;
        }

        private string exportingThreadPath;
        public void buildAndExportThreaded(string filePath)
        {
            this.exportingThreadPath = filePath;
            ThreadStart threadJob = new ThreadStart(this.buildAndExport);
            Thread thread = new Thread(threadJob);
            thread.Start();
            
        }
        private void buildAndExport()
        {

            this.build();
            this.export(exportingThreadPath);
            System.Windows.MessageBox.Show("Exporting done!");

        }



        int pointAccessCounts = 0;

        public void build()
        {
            //Clears previouse grids
            navGrid.Clear();

            //Builds the bitmap image, black for walls, reds for exits
            Graphics gr = Graphics.FromImage(navImage);
            Pen wallPen = new Pen(Brushes.Black, 1);
            Pen exitPen = new Pen(Brushes.Red, 1);
            Brush obsBrush = Brushes.Blue;


            /*
             * BUILD THE WALL GRID
             */
            wallGrid = new GridPoint[Width, Height];

            //Draw walls and exits but exits in white
            gr.Clear(Color.White);
            ObstaclesList.fill(gr, obsBrush);
            ObstaclesList.draw(gr, wallPen);
            WallsList.draw(gr, wallPen);
           
            ExitsList.draw(gr, new Pen(Brushes.White));
            gr.Flush();

            HashSet<GridPoint> wallP = new HashSet<GridPoint>();
            HashSet<GridPoint> exitP = new HashSet<GridPoint>();
            HashSet<GridPoint> obsP = new HashSet<GridPoint>();
            buildGrid(wallGrid, wallP, exitP, obsP);

            //See if the grids are inside or outside
            determineSpatialType(wallGrid, wallP);

            //Then seeks to build forces on to the grid point
            //Gets all the points that is neighbour to the wall
            HashSet<GridPoint> searchSet = wallP;
            HashSet<GridPoint> coveredList = new HashSet<GridPoint>();
            coveredList.UnionWith(wallP);
            for (int wallLevel = 0; wallLevel < this.WallForceDist; wallLevel++)
            {
                searchSet = getSurroundingPoints(false, wallGrid, searchSet, coveredList, null);


                //Goes through all the points
                foreach (GridPoint p in searchSet)
                {
                    int bx = p.X - 1 >= 0 ? p.X - 1 : 0;
                    int by = p.Y - 1 >= 0 ? p.Y - 1 : 0;
                    int ex = p.X + 1 < Width ? p.X + 1 : Width - 1;
                    int ey = p.Y + 1 < Height ? p.Y + 1 : Height - 1;


                    Vector3D forceVect = new Vector3D() { X = 0, Y = 0, Z = 0 };

                    for (int x = bx; x <= ex; x++)
                    {
                        for (int y = by; y <= ey; y++)
                        {
                            if (coveredList.Contains(wallGrid[x, y]))
                            {

                                if (wallGrid[x, y].Type == GridPoint.PointType.Wall || wallGrid[x, y].WallForce > 0)
                                {
                                   
                                        forceVect.X += p.X - x;
                                        forceVect.Y += p.Y - y;
                                   
                                }
                            }
                        }


                    }

                    if (p.SpaType == GridPoint.SpatialType.Outside)
                    {
                        forceVect.X = -forceVect.X;
                        forceVect.Y = -forceVect.Y;
                    }

                    if (forceVect.X != 0 || forceVect.Y != 0 || forceVect.Z != 0)
                    {
                        forceVect.Normalize();
                    }



                    p.WallForceAngle = getAngle(forceVect);

                    int dist = wallLevel + 1;
                    if (this.WallForceDegradeType == ForceDegradationType.Linear)
                    {

                        //Linear degrade
                        p.WallForce = ((1 - ((double)dist / (double)WallForceDist)) * (WallForceMax - WallForceMin)) + WallForceMin;
                    }
                    else
                    {
                        //Inverse square
                        double rootBy = 2 * (dist  -1);
                        if (rootBy > 0)
                            p.WallForce = Math.Pow(WallForceMax, 1 / rootBy);
                        else
                            p.WallForce = WallForceMax;
                    }

                    p.WallX = forceVect.X * p.WallForce;
                    p.WallY = forceVect.Y * p.WallForce;

                    //Console.WriteLine(p.X +  " " + p.Y + " " + forceVect.X + " " + forceVect.Y + " " + p.WallForceAngle);
                }
                
                coveredList.UnionWith(searchSet);
            }

            //Fill the outside walkables  with inward arrows
            //Gets all the inside points
            HashSet<GridPoint> insidePoints = new HashSet<GridPoint>();
            for (int x = 0; x < Width; x++)
            {
                for (int y = 0; y < Height; y++)
                {
                    if (wallGrid[x, y].SpaType == GridPoint.SpatialType.Inside) insidePoints.Add(wallGrid[x, y]);
                }
            }
            HashSet<GridPoint> outsideSearchSet = insidePoints;
            while (true)
            {
                outsideSearchSet = getSurroundingPoints(true, wallGrid, outsideSearchSet, insidePoints, null);
                if (outsideSearchSet.Count < 1) break;

                //Goes through all the points
                foreach (GridPoint p in outsideSearchSet)
                {
                    int bx = p.X - 1 >= 0 ? p.X - 1 : 0;
                    int by = p.Y - 1 >= 0 ? p.Y - 1 : 0;
                    int ex = p.X + 1 < Width ? p.X + 1 : Width - 1;
                    int ey = p.Y + 1 < Height ? p.Y + 1 : Height - 1;


                    Vector3D forceVect = new Vector3D() { X = 0, Y = 0, Z = 0 };

                    for (int x = bx; x <= ex; x++)
                    {
                        for (int y = by; y <= ey; y++)
                        {
                            if (insidePoints.Contains(wallGrid[x, y]))
                            {
                                if (wallGrid[x, y].Type == GridPoint.PointType.Wall || wallGrid[x, y].WallForce > 0)
                                {

                                    forceVect.X += x - p.X;
                                    forceVect.Y += y - p.Y;

                                }
                            }
                        }


                    }

                    if (forceVect.X != 0 || forceVect.Y != 0 || forceVect.Z != 0)
                    {
                        forceVect.Normalize();
                    }



                    p.WallForceAngle = getAngle(forceVect);

                    p.WallForce = this.WallForceMax;

                    p.WallX = forceVect.X * p.WallForce;
                    p.WallY = forceVect.Y * p.WallForce;
                }


                insidePoints.UnionWith(outsideSearchSet);
            }

            //Fill all the obstacle points
            HashSet<GridPoint> obstacleSearch = wallP;
            HashSet<GridPoint> obsCoveredSet = new HashSet<GridPoint>();
            obsCoveredSet.UnionWith(wallP);
            while (true)
            {
                obstacleSearch = this.getSurroundingPoints(true, wallGrid, obstacleSearch, obsCoveredSet, null);
                HashSet<GridPoint> tmpSearch = new HashSet<GridPoint>();
                //Filter it
                foreach (GridPoint p in obstacleSearch)
                {
                    if (p.Type == GridPoint.PointType.Obstacle && !obsCoveredSet.Contains(p))
                    {
                        tmpSearch.Add(p);
                        //Sets the wall force
                        int bx = p.X - 1 >= 0 ? p.X - 1 : 0;
                        int by = p.Y - 1 >= 0 ? p.Y - 1 : 0;
                        int ex = p.X + 1 < Width ? p.X + 1 : Width - 1;
                        int ey = p.Y + 1 < Height ? p.Y + 1 : Height - 1;


                        Vector3D forceVect = new Vector3D() { X = 0, Y = 0, Z = 0 };

                        for (int x = bx; x <= ex; x++)
                        {
                            for (int y = by; y <= ey; y++)
                            {
                                if (insidePoints.Contains(wallGrid[x, y]) && obsCoveredSet.Contains(wallGrid[x,y]))
                                {
                                    if (wallGrid[x, y].Type == GridPoint.PointType.Wall || wallGrid[x,y].Type ==  GridPoint.PointType.Obstacle)
                                    {

                                        forceVect.X += x- p.X;
                                        forceVect.Y += y - p.Y;

                                    }
                                }
                            }
                        }

                        if (forceVect.X != 0 || forceVect.Y != 0 || forceVect.Z != 0)
                        {
                            forceVect.Normalize();
                        }
                        p.WallForceAngle = getAngle(forceVect);
                        p.WallForce = this.WallForceMax;
                        p.WallX = forceVect.X * p.WallForce;
                        p.WallY = forceVect.Y * p.WallForce;

                    }
                }

                if (tmpSearch.Count < 1) break;
                obstacleSearch = tmpSearch;
                obsCoveredSet.UnionWith(obstacleSearch);
            }


            /*
             * BUILDS THE EXIT GRIDS
             * */
            //Build map for each exit
            exitCounts = new Dictionary<int, int>();
            for (int exitId = 0; exitId < ExitsList.Count; exitId++)
            {
                //Statistics
                pointAccessCounts = 0;

                //Instantiates the grid and add it to the global list
                GridPoint[,] layerGrid = new GridPoint[Width, Height];
                navGrid.Add(layerGrid);

                //Draw the image with exit
                gr.Clear(Color.White);
                WallsList.draw(gr, wallPen);
                ObstaclesList.draw(gr, wallPen);
                ExitsList.draw(gr, Pens.White);
                ExitsList[exitId].draw(gr, exitPen);
                gr.Flush();

                //Starts building the grid points for the exit id
                HashSet<GridPoint> wallPoints = new HashSet<GridPoint>();
                HashSet<GridPoint> exitPoints = new HashSet<GridPoint>();
                buildGrid(layerGrid, wallPoints, exitPoints, new HashSet<GridPoint>());

                exitCounts.Add(exitId, exitPoints.Count);

                //Build forces of all areas for the exits
                //Project exit force
                int exitDist = 0;
                searchSet = exitPoints;
                coveredList = new HashSet<GridPoint>();
                coveredList.UnionWith(exitPoints);
                bool searchEnds = false;
                while (!searchEnds)
                {

                    //Start search propagating from an exit
                    searchSet = getSurroundingPoints(false, layerGrid, searchSet, coveredList, null);
                    
                    if (searchSet.Count < 1)
                    {
                        searchEnds = true;
                        break;
                    }

                    exitDist++;

                    foreach (GridPoint p in searchSet)
                    {
                        
                        int diffby = 1;
                        int bx = p.X - diffby >= 0 ? p.X - diffby : 0;
                        int by = p.Y - diffby >= 0 ? p.Y - diffby : 0;
                        int ex = p.X + diffby < Width ? p.X + diffby : Width - 1;
                        int ey = p.Y + diffby < Height ? p.Y + diffby : Height - 1;

                        Vector3D forceVect = new Vector3D() { X = 0, Y = 0, Z = 0 };

                        //Search however many pixels around it excluding the wall
                        HashSet<GridPoint> localSearchSet = new HashSet<GridPoint>();
                        localSearchSet.Add(p);
                        HashSet<GridPoint> localCoveredSet = new HashSet<GridPoint>();
                        localCoveredSet.UnionWith(localSearchSet);

                        bool breakNextTime = false;
                        for (int li = 0; li < SearchbackPixels; li++)
                        {
                            HashSet<GridPoint> localWallPoints = new HashSet<GridPoint>();
                            localSearchSet = getSurroundingPoints(false, layerGrid, localSearchSet, localCoveredSet, localWallPoints);
                            foreach (GridPoint localGp in localSearchSet)
                            {
                                if (coveredList.Contains(localGp))
                                {
                                    //double searchWeight = 1 - (searchbackDistCount / (double)SearchbackPixels);
                                    //forceVect.X += (localGp.X - p.X) * searchWeight;
                                    //forceVect.Y += (localGp.Y - p.Y) * searchWeight;
                                    forceVect.X += localGp.X - p.X;
                                    forceVect.Y += localGp.Y - p.Y;
                                    //forceVect.X += localGp.ExitX;
                                    //forceVect.X += localGp.ExitY;
                                }
                               
                            }
                            
                            localCoveredSet.UnionWith(localSearchSet);
                            if (breakNextTime) break;
                            if (localWallPoints.Count > 0) breakNextTime = true; 
                            if (localSearchSet.Count < 1) break;
                        }
                        //Also remove itself
                        localCoveredSet.Remove(p);



                        //for (int x = bx; x <= ex; x++)
                        //{
                        //    for (int y = by; y <= ey; y++)
                        //    {
                        //        if (wallGrid[x, y].Type == GridPoint.PointType.Wall)
                        //        {
                        //            //If it is a wall or has wall force
                        //            //Vector moves away from the wall
                        //            forceVect.X += 2 * (p.X - x);
                        //            forceVect.Y += 2 * (p.Y - y);
                        //        }
                        //    }
                        //}

                        if (forceVect.X != 0 || forceVect.Y != 0 || forceVect.Z != 0)
                        {
                            forceVect.Normalize();
                        }

                        p.ExitAngle = getAngle(forceVect);
                        if (this.ExitForceDegradeType == ForceDegradationType.Linear)
                        {
                            //Linear degrade
                            p.ExitForce = Math.Max(((1 - ((double)exitDist / (double)ExitForceGradientDist)) * (ExitForceGradientMax - ExitForceGradientMin)) + ExitForceGradientMin, ExitForceGradientMin);
                        }
                        else
                        {
                            //Inverse square
                            double rootBy = 2 * exitDist;
                            p.ExitForce = Math.Max(Math.Pow(ExitForceGradientMax, 1 / rootBy), ExitForceGradientMin);
                        }

                        p.ExitX = forceVect.X * p.ExitForce;
                        p.ExitY = forceVect.Y * p.ExitForce;

                    }

                    coveredList.UnionWith(searchSet);
                }

                ////Applying the smoothing algorithm for testing
                //GridPoint[,] layerCopy = new GridPoint[Width, Height];
                ////Copy it over first
                //for (int w = 0; w < Width; w++)
                //{
                //    for (int h = 0; h < Height; h++)
                //    {
                //        layerCopy[w, h] = layerGrid[w, h];
                //    }
                //}

                //foreach (GridPoint p in coveredList)
                //{
                //    HashSet<GridPoint> pointSearch = new HashSet<GridPoint>();
                //    HashSet<GridPoint> blankCovered = new HashSet<GridPoint>();
                //    pointSearch.Add(p);
                //    blankCovered.UnionWith(pointSearch);
                //    Vector3D vs = new Vector3D();
                //    vs.X = p.ExitX;
                //    vs.Y = p.ExitY;
                //    HashSet<GridPoint> wps = new HashSet<GridPoint>();
                //    while(wps.Count == 0)
                //    {
                //        pointSearch = getSurroundingPoints(false, layerGrid, pointSearch, blankCovered, wps);


                //        foreach (GridPoint sp in pointSearch)
                //        {
                //            vs.X += sp.ExitX;
                //            vs.Y += sp.ExitY;
                //        }

                //        blankCovered.UnionWith(pointSearch);

                //    }
                //    GridPoint np = p.clone();
                //    vs.Normalize();
                //   np.ExitX = vs.X;
                //   np.ExitY = vs.Y;


                //   np.ExitAngle = getAngle(vs);
                //    layerCopy[np.X, np.Y] = np;
                //}
                //navGrid.Add(layerCopy);
                
                

                System.Console.Out.WriteLine("points accessed = " + pointAccessCounts);

            }

         

            /****
             * BUILD HEIGHT MAP
             * */
            if (heightMapImage != null)
            {
                heightGrid = new double[Width, Height];

                for (int x = 0; x < Width; x++)
                {
                    for (int y = 0; y < Height; y++)
                    {
                        //Gets the colour
                        Color c = heightMapImage.GetPixel(x, y);
                        double redValue = (double)c.R;

                        //Puts the height in here
                        GridPoint p = wallGrid[x, Height - 1 - y];
                        if (p.SpaType == GridPoint.SpatialType.Inside)
                        {
                            heightGrid[x, Height - 1 - y] = redValue / 255;
                        }
                        else
                        {
                            heightGrid[x, Height - 1 - y] = 0;
                        }
                        
                    }
                }
            }

            
           
        }

        private void determineSpatialType(GridPoint[,] wallGrid, HashSet<GridPoint> wallP)
        {
            //Assuming that the first pixel in the grid is always outside
            HashSet<GridPoint> searchSet = new HashSet<GridPoint>();
            searchSet.Add(wallGrid[0, 0]);
            HashSet<GridPoint> coveredSet = new HashSet<GridPoint>();
            coveredSet.UnionWith(searchSet);

            
            //Gets all the outside points
            while (true)
            {
                HashSet<GridPoint> searchResult = getSurroundingPoints(false, wallGrid, searchSet, coveredSet, null);
                searchSet = searchResult;
                coveredSet.UnionWith(searchResult);
                if (searchResult.Count < 1) break;
            }

            coveredSet.UnionWith(wallP);

            //Goes through the grid and sets it as inside or outside
            for (int x = 0; x < Width; x++)
            {
                for (int y = 0; y < Height; y++)
                {
                    if (coveredSet.Contains(wallGrid[x, y]))
                    {
                        wallGrid[x, y].SpaType = GridPoint.SpatialType.Outside;
                    }
                    else
                    {
                        wallGrid[x, y].SpaType = GridPoint.SpatialType.Inside;
                    }
                }
            }
            
            

        }

        /// <summary>
        /// Gets the surrounidng points in a local grid
        /// </summary>
        /// <param name="ignoreWalls"></param>
        /// <param name="grid"></param>
        /// <param name="searchSet"></param>
        /// <param name="coveredSet"></param>
        /// <param name="wallPoints">A list of wall points, the algorithm will add the wall points that it finds to this list, used to detect walls within the iteration for backflow smoothing. NULL value skips the wall adding step </param>
        /// <returns></returns>
        public HashSet<GridPoint> getSurroundingPoints(bool ignoreWalls, GridPoint[,] grid, HashSet<GridPoint> searchSet, HashSet<GridPoint> coveredSet, HashSet<GridPoint> wallPoints)
        {
            int width = grid.GetLength(0);
            int height = grid.GetLength(1);
            HashSet<GridPoint> surroundPoints = new HashSet<GridPoint>();
            foreach (GridPoint p in searchSet)
            {
                //Search p in all 8 directions
                int bx = p.X-1 >= 0 ? p.X -1: 0;
                int by = p.Y -1 >= 0 ? p.Y -1: 0;
                int ex = p.X + 1 < width ? p.X + 1 : width -1;
                int ey = p.Y + 1 < height ? p.Y + 1 : height -1;

                if (ignoreWalls)
                {
                    for (int x = bx; x <= ex; x++)
                    {
                        for (int y = by; y <= ey; y++)
                        {
                            pointAccessCounts++;
                            GridPoint sp = grid[x, y];
                            if( !surroundPoints.Contains(sp) && !coveredSet.Contains(sp) )
                            {
                                surroundPoints.Add(sp);
                            }
                        }

                    }
                }
                else
                {
                    for (int x = bx; x <= ex; x++)
                    {
                        for (int y = by; y <= ey; y++)
                        {
                            pointAccessCounts++;
                            GridPoint sp = grid[x, y];
                            if (sp.Type == GridPoint.PointType.Wall && wallPoints != null)
                            {
                                wallPoints.Add(sp);
                            }

                            if (!surroundPoints.Contains(sp) && !coveredSet.Contains(sp) && (sp.Type == GridPoint.PointType.Walkable || sp.Type == GridPoint.PointType.Exit))
                            {
                                int hx = x - p.X;
                                int vy = y - p.Y;
                                if (hx == 0 || vy == 0)
                                {
                                    //Is a straight
                                    surroundPoints.Add(sp);
                                }
                                else
                                {
                                    //Is a diaganol so test for horizontal and vert
                                    if (grid[x, p.Y].Type != GridPoint.PointType.Wall && grid[p.X, y].Type != GridPoint.PointType.Wall)
                                    {
                                        surroundPoints.Add(sp);
                                    }
                                }

                            }
                        }
                    }

                    ////EXTRA TO MAKE IT LOOKUP NON DIAGONAL
                    //int[] xlist = new int[] { -1, 1, -1, 1 };
                    //int[] ylist = new int[] { 1, 1, -1, -1 };
                    //for (int i = 0; i < xlist.Length; i++)
                    //{
                    //    int x = p.X + xlist[i];
                    //    int y = p.Y + ylist[i];

                    //    if (x >= 0 && x < width && y >= 0 && y < height)
                    //    {
                    //        GridPoint sp = grid[x, y];
                    //        if (sp.Type == GridPoint.PointType.Wall && wallPoints != null)
                    //        {
                    //            wallPoints.Add(sp);
                    //        }

                    //        if (!surroundPoints.Contains(sp) && !coveredSet.Contains(sp) && sp.Type == GridPoint.PointType.Walkable)
                    //        {
                    //            int hx = x - p.X;
                    //            int vy = y - p.Y;
                    //            if (hx == 0 || vy == 0)
                    //            {
                    //                //Is a straight
                    //                surroundPoints.Add(sp);
                    //            }
                    //            else
                    //            {
                    //                //Is a diaganol so test for horizontal and vert
                    //                if (grid[x, p.Y].Type != GridPoint.PointType.Wall && grid[p.X, y].Type != GridPoint.PointType.Wall)
                    //                {
                    //                    surroundPoints.Add(sp);
                    //                }
                    //            }

                    //        }
                    //    }

                       

                    //}


                }

            }

            return surroundPoints;

        }

        public System.Windows.Media.Imaging.BitmapSource getWholeImage()
        {
            //Builds the bitmap image, black for walls, reds for exits
            Graphics gr = Graphics.FromImage(navImage);
            Pen wallPen = new Pen(Brushes.Black, 1);
            Pen exitPen = new Pen(Brushes.Red, 1);
            Brush obsBrush = Brushes.Blue;

            //Clear image first
            gr.Clear(Color.White);

            ObstaclesList.fill(gr, obsBrush);
            WallsList.draw(gr, wallPen);
            ExitsList.draw(gr, exitPen);

            gr.Flush();

            navImage.Save("testOutAll.jpg");
            return getForDisplay(navImage);

        }

        public GridPoint[,] getTestSurround()
        {
            Graphics gr = Graphics.FromImage(navImage);
            Pen wallPen = new Pen(Brushes.Black, 1);
            Pen exitPen = new Pen(Brushes.Red, 1);

            //Clear image first
            gr.Clear(Color.White);

            WallsList.draw(gr, wallPen);
            gr.Flush();

            GridPoint[,] gp = new GridPoint[Width,Height];
            HashSet<GridPoint> wallPoints = new HashSet<GridPoint>();
            HashSet<GridPoint> exitPoints = new HashSet<GridPoint>();
            HashSet<GridPoint> searchList = new HashSet<GridPoint>();
            HashSet<GridPoint> coveredList = new HashSet<GridPoint>();
            buildGrid(gp, wallPoints, exitPoints, new HashSet<GridPoint>());

            searchList = wallPoints;
            coveredList.UnionWith(wallPoints);
            
            for( int i = 0; i < 4; i++)
            {
                searchList = getSurroundingPoints(false, gp,searchList, coveredList, null);
                foreach (GridPoint p in searchList)
                {
                    p.WallForce = i;
                }
                coveredList.UnionWith(searchList);

            }

            return gp;

        }


        public System.Windows.Media.Imaging.BitmapSource getExitLayer(int layer)
        {
            if (layer < 0 || layer >= ExitsList.Count) return null;

            //Builds the bitmap image, black for walls, reds for exits
            Graphics gr = Graphics.FromImage(navImage);
            wallPen = new Pen(Brushes.Black, 1);
            exitPen = new Pen(Brushes.Red, 1);

            //Clear image first
            gr.Clear(Color.White);

            ////Draw the walls
            //foreach (NLine line in wallsList)
            //{
            //    gr.DrawLine(wallPen, (float)line.X1, (float)line.Y1, (float)line.X2, (float)line.Y2);
            //}

            ////Draw the exit
            // gr.DrawLine(exitPen, (float)exitsList[layer].X1, (float)exitsList[layer].Y1, (float)exitsList[layer].X2, (float)exitsList[layer].Y2);

            //First gets the grid
            GridPoint[,] grid = navGrid[layer];

            for (int x = 0; x < grid.GetLength(0); x++)
            {
                for (int y = 0; y < grid.GetLength(1); y++)
                {
                    GridPoint p = grid[x, y];
                    switch (p.Type)
                    {
                        case GridPoint.PointType.Exit:
                            gr.FillRectangle(Brushes.Black, new Rectangle(x, y, 1, 1));
                            break;
                        case GridPoint.PointType.Wall:
                            gr.FillRectangle(Brushes.Red, new Rectangle(x, y, 1, 1));
                            break;
                        case GridPoint.PointType.Walkable:
                            //Gets the force from the wall
                            int col = (int) (p.WallForce / WallForceMax * 255);
                            gr.FillRectangle(new SolidBrush( Color.FromArgb(col, col, col)), new Rectangle(x, y, 1, 1));
                            break;

                    }

                }
            }



            //Flush the operations
            gr.Flush();

            return getForDisplay(navImage);
        }

        /// <summary>
        /// </summary>
        /// <param name="pathname">Pathname of the xml to be saved</param>
        public void export(string pathname)
        {
            int lastSlash = pathname.LastIndexOf('/');
            int lastDot = pathname.LastIndexOf('.');
            string pathtoextend = "";
            if (lastDot > -1)
            {
                pathtoextend = pathname.Substring(0, lastDot);
            }
            else
            {
                pathtoextend = pathname;
               
            }
            string imgOutPath = pathtoextend + ".jpg";
            string colAgentOutPath = pathtoextend + "ColAgents.xml";
            string exitpointsCountPath = pathtoextend + "ExitsCount.txt";


            StreamWriter exitsOut = new StreamWriter(new FileStream(exitpointsCountPath, FileMode.Create));
            exitsOut.WriteLine("ExitId  Pixels count");
            foreach (KeyValuePair<int, int> exit in this.exitCounts)
            {
                exitsOut.WriteLine(exit.Key + " " + exit.Value);
                
            }
            exitsOut.Close();

            //Creates the xml file and also save the compare image
            //@"<xagent>
            //<name>"+name+@"</name>
            //<x>" + xo + @"</x>
            //<y>" + yo + @"</y>
            //<collision_x>0.0</collision_x>
            //<collision_y>0.0</collision_y>
            //<exit1_x>0.0</exit1_x>
            //<exit1_y>0.0</exit1_y>
            //<exit2_x>0.0</exit2_x>
            //<exit2_y>0.0</exit2_y>";

            StreamWriter xmlOut = new StreamWriter(new FileStream(pathname, FileMode.Create));
            //xmlOut.Write("<states>"); // commented by MS:09082018

            //Output for agent
            //int agentCounter = 0; // commented by MS:09082018

            Random random = new Random();
            
            for (int i = 0; i < this.NumberOfStartingPedestrians; i++)
            {
                float  velx, vely, steer_x, steer_y, height, speed;
                int x, y, exit;

                bool xyValid = false;
                do{
                    x = random.Next(0, wallGrid.GetLength(0) -1);
                    y = random.Next(0, wallGrid.GetLength(1) -1);
                    if( wallGrid[x,y].Type == GridPoint.PointType.Walkable )
                    {
                        xyValid = true;
                    }
                }while( !xyValid);
                velx = (float)random.NextDouble();
                vely = (float)random.NextDouble();
                steer_x = (float)random.NextDouble();
                steer_y = (float)random.NextDouble();
                height = (float) (heightGrid != null ? heightGrid[x,y] : 1);
                speed = (float)random.NextDouble();
                exit = random.Next(0, navGrid.Count -1 > 0 ? navGrid.Count : 0);
                float xf = (((float)x) / ((float)wallGrid.GetLength(0) - 1))- (float)0.5;
                float yf = (((float)y) / ((float)wallGrid.GetLength(1) - 1)) - (float)0.5;
                
                // Putting states and environment constants  MS:09082018
                xmlOut.Write("<xagent>\n");
                xmlOut.Write("<name>agent</name>\n");
                xmlOut.Write("<x>"+ xf+"</x>\n");
                xmlOut.Write("<y>" + yf + "</y>\n");
                xmlOut.Write("<velx>" + velx + "</velx>\n");
                xmlOut.Write("<vely>" + vely + "</vely>\n");
                xmlOut.Write("<steer_x>" + steer_x + "</steer_x>\n");
                xmlOut.Write("<steer_y>" + steer_y + "</steer_y>\n");
                xmlOut.Write("<height>" + height + "</height>\n");
                xmlOut.Write("<exit>" + exit + "</exit>\n");
                xmlOut.Write("<speed>" + speed + "</speed>\n");
                xmlOut.Write("</xagent>\n");
            }

            //Output for shops
            for (int s = 0; s < this.ShopAgents.Count; s++)
            {
                Agent sagent = this.ShopAgents[s];
                xmlOut.WriteLine("<xagent>");
                xmlOut.WriteLine("<name>shop</name>");
                xmlOut.WriteLine("<id>" + s + "</id>");
                xmlOut.WriteLine("<x>"+sagent.X+"</x>");
                xmlOut.WriteLine("<y>" + sagent.Y + "</y>");
                xmlOut.WriteLine("<z>" + sagent.Z + "</z>");
                xmlOut.WriteLine("</xagent>");
            }

            //Output for Collision agents
            for (int s = 0; s < this.CollisionAgents.Count; s++)
            {
                Agent sagent = this.CollisionAgents[s];
                xmlOut.WriteLine("<xagent>");
                xmlOut.WriteLine("<name>pedestrian_generator</name>");
                xmlOut.WriteLine("<id>" + s + "</id>");
                xmlOut.WriteLine("<last_group_id>"+ (s* 500).ToString() +"</last_group_id>");
                xmlOut.WriteLine("<total_agents>0</total_agents>");
                xmlOut.WriteLine("<production_count>0</production_count>");
                xmlOut.WriteLine("<agent_type>0</agent_type>");
                xmlOut.WriteLine("<x>" + sagent.X + "</x>");
                xmlOut.WriteLine("<y>" + sagent.Y + "</y>");
                xmlOut.WriteLine("<z>" + sagent.Z + "</z>");
                xmlOut.WriteLine("</xagent>");
            }

            //Output for navmap
            int navcounter = 0;
            xmlOut.Write("<states>\n"); // need to be added at the end
            xmlOut.Write("<itno>0<itno>\n");
            xmlOut.Write("<environment>\n");

            
            for (int i = 1; i < navGrid.Count +1; i++) // added by MS:09082018
            {
                xmlOut.Write("\t<EMMISION_RATE_EXIT" + i + ">0</EMMISION_RATE_EXIT" + i + ">\n");

            }

           
            for (int i = 1; i < navGrid.Count +1; i++) // added by MS:09082018
            {
                xmlOut.Write("\t<EXIT" + i + "_PROBABILITY>0</EXIT" + i + "_PROBABILITY>\n");

            }

            for (int i = 1; i < navGrid.Count +1; i++) // added by MS:09082018
            {

                xmlOut.Write("\t<EXIT" + i + "_STATE>0</EXIT" + i + "_STATE>\n");

            }
           
            for (int i = 1; i < navGrid.Count +1 ; i++) // added by MS:09082018
            {

                xmlOut.Write("\t<EXIT" + i + "_CELL_COUNT>0</EXIT" + i + "_CELL_COUNT>\n");

            }

            xmlOut.Write("\t<TIME_SCALER>0.000300</TIME_SCALER>\n");            // added by MS:09082018
            xmlOut.Write("\t<STEER_WEIGHT>0.100000</STEER_WEIGHT>\n");          // added by MS:09082018
            xmlOut.Write("\t<AVOID_WEIGHT>0.020000</AVOID_WEIGHT>\n");          // added by MS:09082018
            xmlOut.Write("\t<COLLISION_WEIGHT>0.500000</COLLISION_WEIGHT>\n");  // added by MS:09082018
            xmlOut.Write("\t<GOAL_WEIGHT>0.200000</GOAL_WEIGHT>\n");            // added by MS:09082018

            xmlOut.Write("</environment>\n\n");// added by MS:09082018
            
            //int count = 0 ; // added by MS21082018

            for (int y = 0; y < Height; y++)
            {
                for (int x = 0; x < Width; x++)
                {
                    double height = heightGrid != null ? heightGrid[x, y] : 1;
                    double newx, newy;// newz; commented by MS:09-08-2018
                    navcounter++;
                    //int temp = 0; commented by MS:09-08-2018
                    //int group_id = 0; commented by MS:09-08-2018
                    newx = (double)x;
                    newy = (double)y;
                    // newz = 0.0; commented by MS:09-08-2018
                   

                    xmlOut.Write("<xagent>\n");
                    xmlOut.Write("<name>navmap</name>\n");
                    //xmlOut.Write("<id>" + count + "</id>\n"); // added by MS21082018
                    xmlOut.Write("<x>" + x + "</x>\n");
                    xmlOut.Write("<y>" + y + "</y>\n");
                    xmlOut.Write("<z0>0</z0>\n");
                    xmlOut.Write("<h>0</h>\n");
                    xmlOut.Write("<qx>0</qx>\n");
                    xmlOut.Write("<qy>0</qy>\n");
                    
                    xmlOut.Write("<height>"+height+"</height>\n");
                    
                    xmlOut.Write("<collision_x>" + wallGrid[x,y].WallX + "</collision_x>\n");
                    xmlOut.Write( "<collision_y>" + wallGrid[x,y].WallY + "</collision_y>\n");
                    
                    int exitLayer = 0;
                    for (int i = 0; i < navGrid.Count; i++)
                    {
                        xmlOut.Write("<exit" + i + "_x>" + navGrid[i][x,y].ExitX + "</exit" + i + "_x>\n");
                        xmlOut.Write("<exit" + i + "_y>" + navGrid[i][x,y].ExitY + "</exit" + i + "_y>\n");
                        //xmlOut.Write("<exit" + i + "_x>" + 0 + "</exit" + i + "_x>\n"); commented by author
                        //xmlOut.Write("<exit" + i + "_y>" + 0 + "</exit" + i + "_y>\n"); commented by author
                        if (navGrid[i][x, y].Type == GridPoint.PointType.Exit)
                        {
                            exitLayer = i + 1;
                        }
                    }

                    //int obsTypeOut;                  **** commented by MS:09-08-2018 ****
                    //switch( wallGrid[x,y].Type)
                    //{
                    //    case GridPoint.PointType.Wall: obsTypeOut = 1; break;
                    //    default : obsTypeOut = 0; break;
                    //}
                    //xmlOut.Write("<production_count>" + temp + "</production_count>\n");
                    //xmlOut.Write("<agent_type>" + temp + "</agent_type>\n");
                    xmlOut.Write("<exit_no>" + exitLayer + "</exit_no>\n");  
                    //xmlOut.Write("<next_possible_id>" + temp + "</next_possible_id>\n");
                    //group_id = exitLayer * 200;

                    //xmlOut.Write("<last_group_id>" + group_id + "</last_group_id>\n");
                    //xmlOut.Write("<created>" + temp + "</created>\n"); 

                   
                    //xmlOut.Write("<obstacle_type>" + obsTypeOut + "</obstacle_type>\n");
                    //xmlOut.Write("<x>" + newx + "</x>\n");
                    //xmlOut.Write("<y>" + newy + "</y>\n");
                    //xmlOut.Write("<z>" + newz + "</z>\n");
                    xmlOut.Write("</xagent>\n");

                     //count++; //added by MS21082018

                  //  navcounter++;
                }
            }


            //Xml out for collision agents
            //if (this.CollisionAgents != null)
            //{
            //    foreach (CollisionAgent col in this.CollisionAgents)
            //    {
            //        double xl = ((2 * (col.X / Width)) - 1);
            //        double yl = (2 * (col.Y / Height) - 1);
            //        double rl = col.Radius / Width;
            //        xmlOut.Write("<xagent>\r\n");
            //        xmlOut.Write("<name>collisionAgent</name>\r\n");
            //        xmlOut.Write("<x>" + xl  + "</x>\r\n");
            //        xmlOut.Write("<y>" + yl + "</y>\r\n");
            //        xmlOut.Write("<radius>" + rl + "</radius>\r\n");
            //        xmlOut.Write("</xagent>\r\n");
            //    }
            //}


            xmlOut.Write("</states>");

           
            xmlOut.Close();


            

            Bitmap outmap = new Bitmap(Width, Height);
            Graphics outGr = Graphics.FromImage(outmap);
            if( backgroundImage != null) outGr.DrawImage(backgroundImage, 0, 0);
            WallsList.draw(outGr, Pens.Black);
            ExitsList.draw(outGr, Pens.Red);
            outGr.Flush();
            outmap.Save(imgOutPath);
        }
        

        public static System.Windows.Media.Imaging.BitmapSource getForDisplay(Bitmap bmp)
        {
            return System.Windows.Interop.Imaging.CreateBitmapSourceFromHBitmap(bmp.GetHbitmap(), IntPtr.Zero, System.Windows.Int32Rect.Empty, System.Windows.Media.Imaging.BitmapSizeOptions.FromEmptyOptions());
        }

        
    }

   
}
