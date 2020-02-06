using System;
using System.IO;
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
using System.Windows.Navigation;
using System.Windows.Shapes;
using Microsoft.Win32;
using System.Runtime.Serialization;
using System.Runtime.Serialization.Formatters.Binary;

namespace PlanEditor
{
    #region Tools

   

    [Serializable]
    public class PeTool
    {
        protected Canvas eCanvas;
        protected List<PeTool> undoTool;
        

        public PeTool(Canvas canvas, List<PeTool> undoTool)
        {
            this.eCanvas = canvas;
            this.undoTool = undoTool;
        }

        public virtual void mouseButtonAction(MouseButtonEventArgs e)
        {
        }

        public virtual void mouseMoveAction(object sender, MouseEventArgs e)
        {
        }

        public virtual bool undo()
        {
            return true;
        }

        public virtual void clear()
        {

        }

        public virtual void toolChangeIn() { }
        public virtual void toolChangeOut() { }

        public virtual void deSerialize(BinaryFormatter formatter, Stream fileStream)
        {

        }

        public virtual void serialize(BinaryFormatter formatter, Stream fileStream)
        {

        }
    }

    [Serializable]
    class AreaRectTool : PeTool
    {
        public System.Windows.Shapes.Path areaRectPath;
        RectangleGeometry areaRectGeom;
        double x1, y1, x2, y2;

        public AreaRectTool(Canvas canvas, List<PeTool> undoTool)
            : base(canvas, undoTool)
        {
            areaRectGeom = new RectangleGeometry();
            areaRectPath = new System.Windows.Shapes.Path();
            areaRectPath.Stroke = System.Windows.Media.Brushes.Red;
            areaRectPath.StrokeThickness = 1;
            areaRectPath.Data = areaRectGeom;
            this.eCanvas.Children.Add(areaRectPath);
        }

        public void setRect(Rect rectangle)
        {
            this.areaRectGeom.Rect = rectangle;
        }

        public Rect getRect()
        {
            return areaRectGeom.Rect;
        }

        public override void clear()
        {
            base.clear();
            areaRectGeom.Rect = new Rect(0, 0, 1, 1);
        }

        private bool pressStarted = false;
        public override void mouseButtonAction(MouseButtonEventArgs e)
        {
            if( e.LeftButton == MouseButtonState.Pressed)
            {
                x1 = e.GetPosition(eCanvas).X;
                y1 = e.GetPosition(eCanvas).Y;
                x2 = x1;
                y2 = y1;

                setRectSize();
                pressStarted = true;
            }
            else if (e.LeftButton == MouseButtonState.Released && pressStarted)
            {
                x2 = e.GetPosition(eCanvas).X;
                y2 = e.GetPosition(eCanvas).Y;

                setRectSize();
                pressStarted = false;
            }

            
            
        }

        private ScrollViewer scroller;
        public override void mouseMoveAction(object sender, MouseEventArgs e)
        {
            scroller = (ScrollViewer) eCanvas.Parent;

            if (sender != null && sender.GetType() == eCanvas.GetType() && e.LeftButton == MouseButtonState.Pressed)
            {
                x2 = e.GetPosition(eCanvas).X;
                y2 = e.GetPosition(eCanvas).Y;

                setRectSize();

                //Resize canvas if getting too near
                if (eCanvas.Width - e.GetPosition(eCanvas).X < 100) eCanvas.Width += 100;
                if (eCanvas.Height - e.GetPosition(eCanvas).Y < 100) eCanvas.Height += 100;

                //Auto scroll if too near the edge
                double x = e.GetPosition(scroller).X;
                double y = e.GetPosition(scroller).Y;
                double xdist = scroller.ActualWidth - x;
                double ydist = scroller.ActualHeight - y;

                double scrollMargin = 40;
                double scrollBy = 0.4;
                if (xdist < scrollMargin)
                {
                    scroller.ScrollToHorizontalOffset(scroller.HorizontalOffset + scrollBy);

                }

                if (x < scrollMargin)
                {
                    scroller.ScrollToHorizontalOffset(scroller.HorizontalOffset - scrollBy);
                }

                if (ydist < scrollMargin)
                {
                    scroller.ScrollToVerticalOffset(scroller.VerticalOffset + scrollBy);
                }

                if (y < scrollMargin)
                {
                    scroller.ScrollToVerticalOffset(scroller.VerticalOffset - scrollBy);
                }

            }

           



         

            
        }

        private void setRectSize()
        {
            
            //if( Math.Abs(x2 - x1) > Math.Abs( y2 - y1 ) )
            //{
                
            //    x2 = x1 + Math.Abs(y2 - y1)*Math.Sign(x2);
            //}
            //else 
            //{
            //    y2 = y1 + Math.Abs(x2 - x1) * Math.Sign(y2); ;
            //}

            if (Math.Abs(x2 - x1) > Math.Abs(y2 - y1))
            {

                x2 = x1 + y2 - y1;
            }
            else
            {
                y2 = y1 + x2 - x1 ;
            }
            areaRectGeom.Rect = new Rect(new System.Windows.Point(x1, y1), new System.Windows.Point(x2, y2));
        }

        public override void serialize(BinaryFormatter formatter, Stream fileStream)
        {
            SRectangle sr = new SRectangle() { x = areaRectGeom.Rect.X, y = areaRectGeom.Rect.Y, width = areaRectGeom.Rect.Width, height = areaRectGeom.Rect.Height };
            formatter.Serialize(fileStream, sr);
        }

        public override void deSerialize(BinaryFormatter formatter, Stream fileStream)
        {
            SRectangle sr = (SRectangle) formatter.Deserialize(fileStream);
            try
            {

                areaRectGeom.Rect = new Rect(sr.x, sr.y, sr.width, sr.height);
            }
            catch (Exception exp)
            {
                areaRectGeom.Rect = new Rect(0, 0, 0, 0);
            }
        }
    }

    [Serializable]
    class LineTool : PeTool
    {
        public List<Polyline> LineList { get; set; }
        public  List<Polyline> LineBackList { get; set; }
        public Brush Stroke { get; set; }
        public double StrokeWidth { get; set; }
        public Brush BackStroke { get; set; }
        public double BackStrokeWidth { get; set; }
        
        public Polyline currentLine;
        public Polyline currentBack;
        private bool lineStarted = false;
        double x1, y1, x2, y2;

        Polyline cLine;
        Point cp;
        public LineTool(Canvas canvas, List<PeTool> undoTool)
            : base(canvas, undoTool)
        {
            LineList = new List<Polyline>();
            LineBackList = new List<Polyline>();
            Stroke = Brushes.Black;
            StrokeWidth = 1;
            BackStroke = Brushes.White;
            BackStrokeWidth = 3;
        }

        public void newLine(double x, double y)
        {
            lineStarted = true;
            currentLine = new Polyline() { Stroke = this.Stroke, StrokeThickness = this.StrokeWidth };
            currentLine.Points.Add(new Point(x, y));
            currentBack = new Polyline() { Stroke = this.BackStroke, StrokeThickness = this.BackStrokeWidth };
            currentBack.Points.Add(new Point(x, y));
            eCanvas.Children.Add(currentBack);
            eCanvas.Children.Add(currentLine);
        }

        public void addLinePoint(double x, double y)
        {
            if (currentLine != null && currentBack != null)
            {
                currentBack.Points.Add(new Point(x, y));
                currentLine.Points.Add(new Point(x, y));
            }
        }

        public void endLine()
        {
            //Commits it to the collection
            if (currentBack != null && currentLine != null)
            {


                if (currentLine.Points.Count > 1)
                {
                    this.LineList.Add(currentLine);
                    this.LineBackList.Add(currentBack);
                }
                else
                {
                    eCanvas.Children.Remove(currentLine);
                    eCanvas.Children.Remove(currentBack);
                }

                currentBack = null;
                currentLine = null;
            }
            lineStarted = false;
        }

        public void cancelLine()
        {
            if (currentLine != null && currentBack != null)
            {
                eCanvas.Children.Remove(currentLine);
                eCanvas.Children.Remove(currentBack);
                currentLine = null;
                currentBack = null;
            }
            lineStarted = false;
        }

        public override void clear()
        {
            base.clear();
            //Do an undo until everything's cleared
            while (undo()) { }
        }

        public override bool undo()
        {
            base.undo();

            if ( currentLine != null && currentLine.Points.Count > 1)
            {
                currentLine.Points.RemoveAt(currentLine.Points.Count -1);
                currentBack.Points.RemoveAt(currentBack.Points.Count - 1);
            }
            else if (currentLine != null && currentLine.Points.Count <= 1)
            {
                cancelLine();
            }
            else
            {
                eCanvas.Children.Remove(LineBackList[LineBackList.Count - 1]);
                eCanvas.Children.Remove(LineList[LineList.Count - 1]);
                LineList.RemoveAt(LineList.Count - 1);
                LineBackList.RemoveAt(LineBackList.Count - 1);
            }

            if (LineList.Count < 1) return false;

            return true;
        }


        
        public override void mouseButtonAction(MouseButtonEventArgs e)
        {
            double x = e.GetPosition(eCanvas).X;
            double y = e.GetPosition(eCanvas).Y;

            if (e.ClickCount == 1 && e.LeftButton == MouseButtonState.Pressed)
            {
                if (lineStarted)
                {
                    this.addLinePoint(x, y);
                }
                else
                {
                    
                    this.newLine(x, y);
                }
            }
            else if ( e.ClickCount > 1 && e.LeftButton == MouseButtonState.Pressed)
            {
                this.endLine();
            }
          
        }

        public override void toolChangeOut()
        {
            base.toolChangeOut();
            //Save currently drawn line
            this.endLine();
        }

        public override void mouseMoveAction(object sender, MouseEventArgs e)
        {
            //if (e.LeftButton == MouseButtonState.Pressed)
            //{
            //    x2 = e.GetPosition(eCanvas).X;
            //    y2 = e.GetPosition(eCanvas).Y;

            //    //if (currentLine != null)
            //    //{
            //    //    currentLine.X2 = x2;
            //    //    currentLine.Y2 = y2;
            //    //    backingLine.X2 = x2;
            //    //    backingLine.Y2 = y2;
            //    //}

            //    if (cp != null)
            //    {
            //        cp.X = x2;
            //        cp.Y = y2;
            //        cLine.UpdateLayout();
            //    }

            if (sender != null && sender.GetType() == eCanvas.Parent.GetType() && lineStarted)
            {
                ScrollViewer editorScroll = (ScrollViewer)sender;
                if (lineStarted)
                {
                    double x = e.GetPosition(editorScroll).X;
                    double y = e.GetPosition(editorScroll).Y;
                    double xdist = editorScroll.ActualWidth - x;
                    double ydist = editorScroll.ActualHeight - y;

                    double scrollMargin = 40;
                    double scrollBy = 0.4;
                    if (xdist < scrollMargin)
                    {
                        editorScroll.ScrollToHorizontalOffset(editorScroll.HorizontalOffset + scrollBy);

                    }

                    if (x < scrollMargin)
                    {
                        editorScroll.ScrollToHorizontalOffset(editorScroll.HorizontalOffset - scrollBy);
                    }

                    if (ydist < scrollMargin)
                    {
                        editorScroll.ScrollToVerticalOffset(editorScroll.VerticalOffset + scrollBy);
                    }

                    if (y < scrollMargin)
                    {
                        editorScroll.ScrollToVerticalOffset(editorScroll.VerticalOffset - scrollBy);
                    }
                }
            }

            if (lineStarted)
            {
                //Resize canvas if getting too near
                if (eCanvas.Width - e.GetPosition(eCanvas).X < 100) eCanvas.Width += 100;
                if (eCanvas.Height - e.GetPosition(eCanvas).Y < 100) eCanvas.Height += 100;
            }
            

           
            //}

        }

       

        public SPolylineCollection convertToSCollection(List<Polyline> lines)
        {
            SPolylineCollection sp = new SPolylineCollection();
            foreach (Polyline poly in lines)
            {
                SPolyLine polyline = new SPolyLine();
                sp.Add(polyline);
                foreach (Point point in poly.Points)
                {
                    polyline.Add(new SPoint() { X = point.X, Y = point.Y });
                }
            }
            return sp;

        }

        public List<Polyline> convertToPolyline(SPolylineCollection sPolylines)
        {
            List<Polyline> polyLines = new List<Polyline>();
            foreach (SPolyLine sPolyline in sPolylines)
            {
                Polyline polyline = new Polyline();
                polyLines.Add(polyline);
                foreach (SPoint sPoint in sPolyline)
                {
                    polyline.Points.Add(new Point(sPoint.X, sPoint.Y));
                }
            }
            return polyLines;

        }

        public override void deSerialize(BinaryFormatter formatter, Stream fileStream)
        {
            base.deSerialize(formatter, fileStream);
            List<Polyline> lines = this.convertToPolyline((SPolylineCollection)formatter.Deserialize(fileStream));
            foreach (Polyline polyline in lines)
            {
                Polyline backLine = new Polyline() { Stroke = this.BackStroke, StrokeThickness = this.BackStrokeWidth};
                Polyline foreLine = new Polyline(){ Stroke = this.Stroke, StrokeThickness = this.StrokeWidth};
                backLine.Points = polyline.Points;
                foreLine.Points = polyline.Points;
                this.LineBackList.Add(backLine);
                this.LineList.Add(foreLine);
                this.eCanvas.Children.Add(backLine);
                this.eCanvas.Children.Add(foreLine);
            }
            //List<SLine> lines = (List<SLine>)formatter.Deserialize(fileStream);
            //foreach (SLine line in lines)
            //{
            //    Line backLine = new Line() { Stroke = this.BackStroke, StrokeThickness = this.BackStrokeWidth, X1 = line.x1, X2 = line.x2, Y1 = line.y1, Y2 = line.y2 };
            //    Line foreLine =  new Line() 
            //    this.LineBackList.Add(backLine);
            //    this.LineList.Add(foreLine);
            //    this.eCanvas.Children.Add(backLine);
            //    this.eCanvas.Children.Add(foreLine);
            //}
        }

        public override void serialize(BinaryFormatter formatter, Stream fileStream)
        {
            base.serialize(formatter, fileStream);
            formatter.Serialize(fileStream, this.convertToSCollection(this.LineList));
             //Builds a list of lines
            //List<SLine> lines = new List<SLine>();
            //foreach (Line line in this.LineList)
            //{
            //    lines.Add(new SLine() { x1 = line.X1, x2 = line.X2, y1 = line.Y1, y2 = line.Y2 });
            //}
            //formatter.Serialize(fileStream, lines);
        }
    }

    [Serializable]
    class WallTool : LineTool
    {
        public WallTool(Canvas canvas, List<PeTool> undoTool)
            : base(canvas, undoTool)
        {

        }
    }

    [Serializable]
    class ExitTool : LineTool
    {
        public List<Polyline> WallsList {get; set;}
       
        public ExitTool(Canvas canvas, List<PeTool> undoTool)
            : base(canvas, undoTool)
        {
        }
       
    }

    [Serializable]
    class ObstacleTool : LineTool
    {
         public ObstacleTool(Canvas canvas, List<PeTool> undoTool)
            : base(canvas, undoTool)
        {

        }
    }


    [Serializable]
    public class ShopAgentTool : PeTool
    {
        public List<Agent> shopAgents;
        public List<Ellipse> ellipses;

        Agent shop;
        Ellipse ellipse;

        Brush strokeColour = Brushes.White;
        Brush fillColour = Brushes.Blue;
        double ellipseStroke = 0.5;

         public ShopAgentTool(Canvas canvas, List<PeTool> undoTool)
            : base(canvas, undoTool)
        {
            shopAgents = new List<Agent>();
            ellipses = new List<Ellipse>();
            
        }

        public void addAgent(double x, double y)
        {
            //shop = new Agent() { X = x, Y = y , Radius = 1, Force = 1};
            //shopAgents.Add(shop);
            //ellipse = addEllipse(shop);

            shop = new Agent() { X = x, Y = y, Radius = 3, Force = 1};
            shopAgents.Add(shop);
            ellipse = addEllipse(shop);
        }

        public Ellipse addEllipse(Agent agent)
        {
            Ellipse localEllipse = new Ellipse() { Stroke = strokeColour, StrokeThickness = ellipseStroke, Fill = fillColour};
            localEllipse.RenderTransform = new TranslateTransform(agent.X, agent.Y );
            localEllipse.Width = agent.Radius * 2;
            localEllipse.Height = agent.Radius * 2;
            TranslateTransform transform = (TranslateTransform)localEllipse.RenderTransform;
            transform.X = agent.X - agent.Radius;
            transform.Y = agent.Y - agent.Radius;
            ellipses.Add(localEllipse);
            eCanvas.Children.Add(localEllipse);
            eCanvas.UpdateLayout();
            return localEllipse;
        }

        public void updateEllipseRadius(Agent agent, Ellipse el)
        {
            el.Height = agent.Radius * 2;
            el.Width = agent.Radius * 2;
            TranslateTransform t = (TranslateTransform)el.RenderTransform;
            t.X = agent.X - agent.Radius;
            t.Y = agent.Y - agent.Radius;
        }

        public void updateAgentRadius(double x, double y)
        {
            if(shop != null)
            { 
                double radius = Math.Sqrt(Math.Pow(shop.X - x, 2) + Math.Pow(shop.Y - y, 2));
                shop.Radius = radius;
                updateEllipseRadius(shop, ellipse);
            }
           
        }

        public void updateAgentForce(double x, double y)
        {

        }

        public override void clear()
        {
            base.clear();
            while (undo()) { }
        }

        public override bool undo()
        {
            
            if (!placingAgent)
            {
                if (this.shopAgents.Count > 0 && this.ellipses.Count > 0)
                {
                    this.shopAgents.RemoveAt(this.shopAgents.Count - 1);
                    this.eCanvas.Children.Remove(this.ellipses[this.ellipses.Count - 1]);
                    this.ellipses.RemoveAt(this.ellipses.Count - 1);
                    return true;
                }
                else
                {
                    return false;
                }
            }

            return false;

        }

        

     

        private bool placingAgent;
        public override void mouseButtonAction(MouseButtonEventArgs e)
        {
            //if (e.LeftButton == MouseButtonState.Pressed)
            //{
            //    if (!placingAgent)
            //    {
            //        placingAgent = true;
            //        addAgent(e.GetPosition(eCanvas).X, e.GetPosition(eCanvas).Y);
            //    }
            //}
            //else
            //{
            //    placingAgent = false;
            //}

            if (e.LeftButton == MouseButtonState.Pressed && e.ClickCount > 0)
            {
                addAgent(e.GetPosition(eCanvas).X, e.GetPosition(eCanvas).Y);
                //updateAgentRadius(2, 2);
            }


        }

        public override void mouseMoveAction(object sender, MouseEventArgs e)
        {
            if (placingAgent)
            {
               // updateAgentRadius(e.GetPosition(eCanvas).X, e.GetPosition(eCanvas).Y);
            }
        }

        public override void toolChangeIn()
        {
            base.toolChangeIn();
        }

        public override void toolChangeOut()
        {
            base.toolChangeOut();
        }

        public override void deSerialize(BinaryFormatter formatter, Stream fileStream)
        {
            base.deSerialize(formatter, fileStream);
            this.shopAgents = (List<Agent>) formatter.Deserialize(fileStream);
            foreach (Agent col in this.shopAgents)
            {
                Ellipse temp = this.addEllipse(col);
                this.updateEllipseRadius(col, temp);

            }
        }

        public override void serialize(BinaryFormatter formatter, Stream fileStream)
        {
            base.serialize(formatter, fileStream);
            formatter.Serialize(fileStream, this.shopAgents);
        }
    }

    [Serializable]
    public class CollisionAgentTool : PeTool
    {
        public List<Agent> collisionAgents;
        public List<Ellipse> ellipses;
    
        Agent col;
        Ellipse ellipse;


        Brush strokeColour = Brushes.Purple;
        Brush fillColour = Brushes.Brown;
        double ellipseStroke = 1;


        public CollisionAgentTool(Canvas canvas, List<PeTool> undoTool)
            : base(canvas, undoTool)
        {
            collisionAgents = new List<Agent>();
            ellipses = new List<Ellipse>();
            
        }

        public void addAgent(double x, double y)
        {
            col = new Agent() { X = x, Y = y , Radius = 1, Force = 1};
            collisionAgents.Add(col);
            ellipse = addEllipse(col);
        }

        public Ellipse addEllipse(Agent agent)
        {
            Ellipse localEllipse = new Ellipse() { Stroke = strokeColour, StrokeThickness = ellipseStroke, Fill = fillColour };
            localEllipse.RenderTransform = new TranslateTransform(agent.X, agent.Y );
            ellipses.Add(localEllipse);
            eCanvas.Children.Add(localEllipse);
            return localEllipse;
        }

        public void updateEllipseRadius(Agent agent, Ellipse el)
        {
            el.Height = agent.Radius * 2;
            el.Width = agent.Radius * 2;
            TranslateTransform t = (TranslateTransform)el.RenderTransform;
            t.X = agent.X - agent.Radius;
            t.Y = agent.Y - agent.Radius;
        }

        public void updateAgentRadius(double x, double y)
        {
            if(col != null)
            { 
                double radius = Math.Sqrt(Math.Pow(col.X - x, 2) + Math.Pow(col.Y - y, 2));
                col.Radius = radius;
                updateEllipseRadius(col, ellipse);
            }
           
        }

        public void updateAgentForce(double x, double y)
        {

        }

        public override void clear()
        {
            base.clear();
            while (undo()) { }
        }

        public override bool undo()
        {
            
            if (!placingAgent)
            {
                if (this.collisionAgents.Count > 0 && this.ellipses.Count > 0)
                {
                    this.collisionAgents.RemoveAt(this.collisionAgents.Count - 1);
                    this.eCanvas.Children.Remove(this.ellipses[this.ellipses.Count - 1]);
                    this.ellipses.RemoveAt(this.ellipses.Count - 1);
                    return true;
                }
                else
                {
                    return false;
                }
            }

            return false;

        }

        

     

        private bool placingAgent;
        public override void mouseButtonAction(MouseButtonEventArgs e)
        {
            if (e.LeftButton == MouseButtonState.Pressed)
            {
                if (!placingAgent)
                {
                    placingAgent = true;
                    addAgent(e.GetPosition(eCanvas).X, e.GetPosition(eCanvas).Y);
                }
            }
            else
            {
                placingAgent = false;
            }
        }

        public override void mouseMoveAction(object sender, MouseEventArgs e)
        {
            if (placingAgent)
            {
                updateAgentRadius(e.GetPosition(eCanvas).X, e.GetPosition(eCanvas).Y);
            }
        }

        public override void toolChangeIn()
        {
            base.toolChangeIn();
        }

        public override void toolChangeOut()
        {
            base.toolChangeOut();
        }

        public override void deSerialize(BinaryFormatter formatter, Stream fileStream)
        {
            base.deSerialize(formatter, fileStream);
            this.collisionAgents = (List<Agent>) formatter.Deserialize(fileStream);
            foreach (Agent col in this.collisionAgents)
            {
                Ellipse temp = this.addEllipse(col);
                this.updateEllipseRadius(col, temp);

            }
        }

        public override void serialize(BinaryFormatter formatter, Stream fileStream)
        {
            base.serialize(formatter, fileStream);
            formatter.Serialize(fileStream, this.collisionAgents);
        }
    }

    #endregion

    class PlanEditManager
    {

        #region Planeditvars

        //Main init variables
        private Canvas eCanvas;
        private System.Windows.Shapes.Rectangle focusRect;
        private System.Windows.Controls.Image referenceImage;
        private System.Windows.Controls.Image hightmapImage;
        private ScaleTransform canvasTransform;
        private string imageFilePath;
        private string heightmapFilePath;

        private int _height, _width;
        public int DefinedWith 
        {
            get
            {
                return _width;
            }
            set
            {
                _width = value;
               
            }
        }

        public int DefinedWithProportional
        {
            get
            {
                return _width;
            }
            set
            {
                _width = value;
                //Sets proportional
                _height = System.Convert.ToInt32((double)_width / (areaRectTool.getRect().Width / areaRectTool.getRect().Height));
            }
        }

        public int DefinedHeight
        {
            get
            {
                return _height;
            }

            set
            {
                _height = value;
               
            }
            
        }

        public int DefinedHeightProportional
        {
            get
            {
                return _height;
            }

            set
            {
                _height = value;
                //Sets proportional
                _width = System.Convert.ToInt32((double)_height * (areaRectTool.getRect().Width / areaRectTool.getRect().Height));
            }

        }

        #endregion


        //Tools
        public enum CurrentTool
        {
            AreaRect, Wall, Exit, Area, CollisionAgent, Obstacle, Shop
        };
        private Dictionary<CurrentTool, PeTool> toolsCollection;
        private CurrentTool currentToolEnum;
        private PeTool currentTool;
        private AreaRectTool areaRectTool;
        private WallTool wallTool;
        private ExitTool exitTool;
        private ObstacleTool obsTool;
        private CollisionAgentTool colAgentTool;
        private ShopAgentTool shopAgentTool;
        private List<PeTool> undoList = new List<PeTool>();

        /// <summary>
        /// Plan editor that is linked to a canvas
        /// </summary>
        /// <param name="canvas"></param>
        public PlanEditManager(Canvas canvas)
        {
            this.eCanvas = canvas;

            //Rectangle used to draw focus
            focusRect = new System.Windows.Shapes.Rectangle() { Width = canvas.Width, Height = canvas.Height };
            focusRect.Fill = System.Windows.Media.Brushes.White;
            eCanvas.Children.Add(focusRect);

            //Reference image 
            referenceImage = new System.Windows.Controls.Image();
            referenceImage.SizeChanged += new SizeChangedEventHandler(imageSizeChange);
            eCanvas.Children.Add(referenceImage);

            hightmapImage = new System.Windows.Controls.Image();
            hightmapImage.SizeChanged += new SizeChangedEventHandler(imageSizeChange);
            eCanvas.Children.Add(hightmapImage);
            
            //Sets up the canvas transform for zooming in and out
            canvasTransform = new ScaleTransform();
            eCanvas.LayoutTransform = canvasTransform;

            //Initialize tools for use
            toolsCollection = new Dictionary<CurrentTool, PeTool>();
            areaRectTool = new AreaRectTool(eCanvas, undoList);
            wallTool = new WallTool(eCanvas, undoList  );
            exitTool = new ExitTool(eCanvas, undoList) { Stroke = Brushes.Red };
            exitTool.WallsList = wallTool.LineList; //Make them use the same walls object
            obsTool = new ObstacleTool(eCanvas, undoList) { Stroke = Brushes.Blue };
            colAgentTool = new CollisionAgentTool(eCanvas, undoList);
            shopAgentTool = new ShopAgentTool(eCanvas, undoList);
            
            //Register them
            toolsCollection[CurrentTool.AreaRect] = areaRectTool;
            toolsCollection[CurrentTool.Wall] = wallTool;
            toolsCollection[CurrentTool.Exit] = exitTool;
            toolsCollection[CurrentTool.Obstacle] = obsTool;
            toolsCollection[CurrentTool.CollisionAgent] = colAgentTool;
            toolsCollection[CurrentTool.Shop] = shopAgentTool;
            setCurrentTool(CurrentTool.AreaRect);
           
            //Subscribe canvas events
            this.eCanvas.SizeChanged += new SizeChangedEventHandler(eCanvas_SizeChanged);
            this.eCanvas.MouseMove += new MouseEventHandler(mouseMoveActions);
            this.eCanvas.MouseWheel += new MouseWheelEventHandler(eCanvas_MouseWheel);
            this.eCanvas.MouseUp += new MouseButtonEventHandler(mouseButtonActions);
            this.eCanvas.MouseDown += new MouseButtonEventHandler(mouseButtonActions);
            
            
        }

        public void matchAreaToImage()
        {
            if(this.imageFilePath != null)
            {
                this.areaRectTool.setRect(new Rect(0, 0, this.referenceImage.Source.Width, this.referenceImage.Source.Height));
            }
            

        }
        
        public void clear()
        {
            
        }

        public void save(string pathname)
        {
            Stream stream = File.Open(pathname, FileMode.Create);
            BinaryFormatter formatter = new BinaryFormatter();

            foreach (KeyValuePair<CurrentTool,PeTool> t in toolsCollection)
            {
                t.Value.serialize(formatter, stream);
            }
            //Saves image paths
            formatter.Serialize(stream, new StringHolder() { Value = imageFilePath });
            formatter.Serialize(stream, new StringHolder() { Value = heightmapFilePath });
            stream.Close();
          
            
        }

        public void load(string pathname)
        {
            Stream stream = File.Open(pathname, FileMode.Open);
            BinaryFormatter formatter = new BinaryFormatter();
            foreach (KeyValuePair<CurrentTool, PeTool> t in toolsCollection)
            {
                t.Value.deSerialize(formatter,stream);
            }
            imageFilePath = ((StringHolder)formatter.Deserialize(stream)).Value;
            heightmapFilePath = ((StringHolder)formatter.Deserialize(stream)).Value;
            stream.Close();
            //Loads the image
            if (imageFilePath != null) loadBackgroundImage(imageFilePath);
            if (heightmapFilePath != null) loadHeightmapImage(heightmapFilePath);
        }

        /// <summary>
        /// Zooms the canvas with the mouse wheel
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        void eCanvas_MouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (e.Delta > 0)
            {
                this.zoom(2);
            }
            else
            {
                this.zoom(0.5);
            }

        }

        
        public void mouseButtonActions(object sender, MouseButtonEventArgs e)
        {

            this.currentTool.mouseButtonAction(e);
            
        }

        public void mouseMoveActions(object sender, MouseEventArgs e)
        {
            this.currentTool.mouseMoveAction(sender, e);
        }


        public CurrentTool getCurrentTool()
        {
            return this.currentToolEnum;
        }

        public void undo()
        {
            //if (this.undoList.Count > 0)
            //{
            //    this.undoList[this.undoList.Count - 1].undo();
            //    this.undoList.RemoveAt(this.undoList.Count - 1);
            //}
            this.currentTool.undo();
        }

        public void setCurrentTool(CurrentTool tool)
        {
            this.currentToolEnum = tool;
            if (this.currentTool != null) this.currentTool.toolChangeOut();
            this.currentTool = toolsCollection[tool];
            this.currentTool.toolChangeIn();
            //Console.WriteLine(this.currentTool.ToString());
        }

     

        public void zoom(double scale)
        {
            canvasTransform.ScaleX *= scale;
            canvasTransform.ScaleY *= scale;
        }

        public void alternateReferences()
        {
            if (heightmapFilePath != null & imageFilePath != null)
            {
                if (hightmapImage.Visibility == Visibility.Visible)
                {
                    hightmapImage.Visibility = Visibility.Hidden;
                    referenceImage.Visibility = Visibility.Visible;
                }
                else
                {
                    hightmapImage.Visibility = Visibility.Visible;
                    referenceImage.Visibility = Visibility.Hidden;
                }
            }
        }


        public void loadHeightmapImage(string file)
        {
            heightmapFilePath = file;
            try
            {
                hightmapImage.Source = new BitmapImage(new Uri(file));
            }
            catch (Exception e)
            { }
        }
       

        public void loadBackgroundImage(string file)
        {
            imageFilePath = file;
            try
            {
                referenceImage.Source = new BitmapImage(new Uri(file));
            }
            catch (Exception e)
            {

            }
        }

        /// <summary>
        /// Matches the blank square to the canvas size in order to
        /// get clicks
        /// </summary>
        private void resizeSelectSquare()
        {
            focusRect.Height = eCanvas.Height;
            focusRect.Width = eCanvas.Width;
        }

        /// <summary>
        /// Tries to change canvas size and the canvas will change size if bigger
        /// </summary>
        /// <param name="width"></param>
        /// <param name="height"></param>
        private void resizeCanvas(double width, double height)
        {
            if (this.eCanvas.Width < width) this.eCanvas.Width = width;
            if (this.eCanvas.Height < height) this.eCanvas.Height = height;
        }

        public NavMap buildMap(int width, int height)
        {
            if (width > 0 && height > 0)
            {
                this.DefinedWith = width;
                this.DefinedHeight = height;
            }
            else if (width < 0) this.DefinedHeightProportional = height;
            else if (height < 0) this.DefinedWithProportional = width;
           

            //Create a grid
            NavMap navMap = new NavMap(this.DefinedWith, this.DefinedHeight);


            //Define scaling factors
            double startX = areaRectTool.getRect().X;
            double startY = areaRectTool.getRect().Y;
            double actualWidth = areaRectTool.getRect().Width;
            double actualHeight = areaRectTool.getRect().Height;
            double defWidth = this.DefinedWith;
            double defHeight = this.DefinedHeight;
            double xScale = defWidth / actualWidth;
            double yScale = defHeight / actualHeight;

            //Gets the walls and Exits
            navMap.WallsList = this.wallTool.convertToSCollection(this.wallTool.LineList).getDisplace(startX, startY,xScale,yScale);
            navMap.ExitsList = this.exitTool.convertToSCollection(this.exitTool.LineList).getDisplace(startX, startY, xScale, yScale);

            //Sets the obstacles
            navMap.ObstaclesList = this.obsTool.convertToSCollection(this.obsTool.LineList).getDisplace(startX, startY, xScale, yScale);

            //Sets the collision agents
            List<Agent> colAgents = new List<Agent>();
            foreach (Agent agent in this.colAgentTool.collisionAgents)
            {
                Agent tempAgent = agent.getDisplace(startX, startY, xScale);
                //Must also invert y
                tempAgent.Y = navMap.Height - tempAgent.Y;
                colAgents.Add(tempAgent);
            }
            navMap.CollisionAgents = colAgents;

           
            
            //Sets the shop agents
            List<Agent> shopAgents = new List<Agent>();
            foreach (Agent agent in this.shopAgentTool.shopAgents)
            {
                Agent tempAgent = agent.getDisplace(startX, startY, xScale);
                //Must also invert y
                tempAgent.Y = navMap.Height - tempAgent.Y;
                shopAgents.Add(tempAgent);
            }
            navMap.ShopAgents = shopAgents;
           
            

            //Sets the backround and height map
            
            navMap.setBackgroundImage(imageFilePath, areaRectTool.getRect());
            navMap.setHeightMapImage(heightmapFilePath, areaRectTool.getRect());
            return navMap;
       
            
        }

        #region MiscEvent
        void eCanvas_SizeChanged(object sender, SizeChangedEventArgs e)
        {
            this.resizeSelectSquare();
        }

        void imageSizeChange(object sender, SizeChangedEventArgs e)
        {
            resizeCanvas(e.NewSize.Width, e.NewSize.Height);
        }

        #endregion
    }
}
