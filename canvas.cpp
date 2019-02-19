#include "canvas.h"

const QPointF CIRCLE_CENTRE = QPointF(50,50);
const double RADIUS = 35.0;
ChordDiagram *o_diagram = new ChordDiagram();



Canvas::Canvas(QWidget *parent) : QOpenGLWidget(parent)
{
    setMouseTracking(true);
//    QSizePolicy qsp(QSizePolicy::Preferred,QSizePolicy::Preferred);
//    if(this->height() > this->width())
//    {
//        qsp.setWidthForHeight(true);
//    }else
//    {
//        qsp.setHeightForWidth(true);
//    }
//    //  qsp.setHeightForWidth(true);
    //    setSizePolicy(qsp);
}

//void Canvas::resizeEvent(QResizeEvent *event)
//{
//    event->accept();

//    if(event->size().width() > event->size().height()){
//        QOpenGLWidget::resize(event->size().height(),event->size().height());
//    }else{
//        QOpenGLWidget::resize(event->size().width(),event->size().width());
//    }
//}

void Canvas::initializeGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_BLEND);
    //Required blending for alpha blending
    glEnable(GL_ALPHA);
    //used to add alpha values
    //renders using stencil buffer for concave polygons
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    //set canvas background to white
    glClearColor(1.0,1.0,1.0,1.0);
}

void Canvas::resizeGL(int w, int h)
{
    glViewport(0,0,w,h);
}

void Canvas::paintGL()
{
    prepareDraw();
    float numSegments = calculateNumCircleSegments(RADIUS);
    drawCircle(CIRCLE_CENTRE.x(), CIRCLE_CENTRE.y(), RADIUS, numSegments);
    placeArcs();
    o_diagram->CalculateCentreOfArcs();
    placeCentrePoints();
    placeLinks();
    placeBoundingBoxes();
}


void Canvas::prepareDraw()
{
    glPointSize(1);
    setOrtho();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Canvas::setOrtho()
{
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glClearColor(1.0,1.0,1.0,1.0);
    glOrtho(0,100,100,0,-1,1);
    glClear(GL_COLOR_BUFFER_BIT);
}

void Canvas::placeArcs()
{
    //qDebug() << "Placing Arcs";

    std::vector<Node> nodes = o_diagram->GetNodes();
    int numOfArcs = o_diagram->GetNumberOfNodes();
    float sizeOfArcNoGap = 2.0f * M_PI / numOfArcs;
    float gap = sizeOfArcNoGap/100*20;
    float sizeOfArc = sizeOfArcNoGap-gap;

    float startAngle = 0;
    float numSegments = calculateNumCircleSegments(RADIUS);

    for(int i=0; i<numOfArcs; ++i)
    {
        Node &node = nodes.at(i);
        startAngle = i*sizeOfArcNoGap;
        node.SetPolarStart(startAngle);
        node.SetPolarSpan(sizeOfArc);
        node.SetPolarEnd(startAngle+sizeOfArc);

        bool highlighted = node.GetHighlighted();

        //qDebug() << startAngle << sizeOfArc << startAngle+sizeOfArc;
        drawArc(CIRCLE_CENTRE.x(), CIRCLE_CENTRE.y(), RADIUS, node.GetPolarStart(), node.GetPolarSpan(), numSegments, highlighted);



    }
    //pass in the array with the updated values to the object
    o_diagram->SetNodes(nodes);
}

void Canvas::placeCentrePoints()
{
    //qDebug() << "Placing Centre Points";

    std::vector<Node> nodes = o_diagram->GetNodes();

    if(GetCenterOfArcsToggle())
    {
        for(auto &it : nodes)
        {
            QPointF centre = it.GetCartCentre();
            drawCentreOfArc(centre);
        }
    }
}

void Canvas::placeLinks()
{
    //qDebug() << "Placing links";

    std::vector<Link> links = o_diagram->GetLinks();
    std::vector<Node> nodes = o_diagram ->GetNodes();

    for(auto &it : links)
    {
        QString input = it.GetInputName();
        QString output = it.GetOutputName();
        QPointF inputCentre;
        QPointF outputCentre;

        auto nItI = std::find_if(nodes.begin(), nodes.end(), [&input] (Node &obj)
        {
            return obj.GetName() == input;
        });

        if(nItI != nodes.end())
        {
            inputCentre = nItI->GetCartCentre();
        }

        auto nItO = std::find_if(nodes.begin(), nodes.end(), [&output] (Node &obj)
        {
            return obj.GetName() == output;
        });

        if(nItO != nodes.end())
        {
            outputCentre = nItO->GetCartCentre();
        }

        drawBezierLink(inputCentre, outputCentre);
    }


}

void Canvas::placeBoundingBoxes()
{
    o_diagram->CalculateBoundingBoxes();

    if(GetBoundingBoxToggle())
    {
        std::vector<Node> nodes = o_diagram ->GetNodes();
        for(auto &it : nodes)
        {
            QRectF box = it.GetRectContainer();
            drawBoundingBoxes(box);
        }
    }


}

//calculates the number of segments that will compose the circle
float Canvas::calculateNumCircleSegments(float r)
{
    return 10 * sqrtf(r); //change10 to increase/decrease precision
}

QPointF Canvas::calculateBezierePoint(QPointF input, QPointF output, QPointF control, double t)
{
    //qDebug() << "Calculating";

    float aX = input.x();
    float aY = input.y();
    float bX = control.x();
    float bY = control.y();
    float cX = output.x();
    float cY = output.y();

    float pX = aX + 2*t*(bX-aX) + t*t*(cX-(2*bX)+aX);
    float pY = aY + 2*t*(bY-aY) + t*t*(cY-(2*bY)+aY);

    float px = qPow((1-t),3) * aX + 3 * t * qPow((1-t),2)*bX + 3* qPow(t,2) * (1-t) * cX + qPow(t,3) * cX;
    float py = qPow((1-t),3) * aY + 3 * t * qPow((1-t),2)*bY + 3* qPow(t,2) * (1-t) * cY + qPow(t,3) * cY;

    return QPointF(px, py);
}

void Canvas::drawCircle(float cx, float cy, float r, int numSegments)
{
    //qDebug()<<"Drawing Circle";

    float theta = 2.0f * M_PI / float(numSegments);
        float c = cosf(theta);//precalculate the sine and cosine
        float s = sinf(theta);
        float t;

        float x = r;//we start at angle = 0
        float y = 0;

        glLineWidth(1);
        glBegin(GL_LINE_LOOP);
        for(int i = 0; i < numSegments; i++)
        {
            glColor3d(1.0,0.0,1.0);
            glVertex2f(x + cx, y + cy);//output vertex

            //apply the rotation matrix
            t = x;
            x = c * x - s * y;
            y = s * t + c * y;
        }
        glEnd();
}


void Canvas::drawArc(float cx, float cy, float r, float start_angle, float arc_angle, int num_segments, bool highlighted)
{
    //qDebug() << "Drawing Arcs -----------------" << highlighted;

    float theta = arc_angle / float(num_segments -1);
    //theta is now calculated from the arc angle instead, the - 1 bit comes from the fact that the arc is open

    float tangetial_factor = tanf(theta);
    float radial_factor = cosf(theta);

    float x = r * cosf(start_angle);
    //we now start at the start angle
    float y = r * sinf(start_angle);

    glLineWidth(5);
    glBegin(GL_LINE_STRIP);
    //since the arc is not a closed curve, this is a strip now
    for(int ii = 0; ii < num_segments; ii++)
    {
        if(highlighted)
        {
            glColor4d(1,0.5,0.0,1.0);
            //qDebug() << start_angle;
        }else
        {
            glColor4d(1,0.5,0.0,0.5);
        }


        glVertex2f(x + cx, y + cy);

        float tx = -y;
        float ty = x;

        x += tx * tangetial_factor;
        y += ty * tangetial_factor;

        x *= radial_factor;
        y *= radial_factor;
    }
    glEnd();
}

void Canvas::drawCentreOfArc(QPointF c)
{
    //qDebug() << "Drawing Centre Points";

    glPointSize(5);
    glBegin(GL_POINTS);
    glColor3d(0.0, 1.0, 0.0);
    glVertex2f(c.x(), c.y());


    glEnd();
}

void Canvas::drawLink(QPointF input, QPointF output)
{
    //qDebug() << "Drawing Links";

    glLineWidth(2);
    glColor3d(0.0,0.0,1.0);
    glBegin(GL_LINE_STRIP);
    glVertex2f(input.x(), input.y());
    glVertex2f(output.x(), output.y());

    glEnd();
}

void Canvas::drawBezierLink(QPointF input, QPointF output)
{
    for(double i =0; i<1; i+= 0.01)
    {
        QPointF control = calculateBezierePoint(input, output, CIRCLE_CENTRE, i);
        QPointF control2 = calculateBezierePoint(input, output, CIRCLE_CENTRE, i+0.01);

        drawLink(control, control2);

    }

}

void Canvas::drawBoundingBoxes(QRectF box)
{
    //qDebug() << "Drawing Box";
    QPointF a = box.topLeft();
    QPointF b = box.topRight();
    QPointF c = box.bottomRight();
    QPointF d = box.bottomLeft();
    glColor3f(0.0, 1.0, 0.0);
    glLineWidth(3);
    glBegin(GL_LINE_LOOP);
    glVertex2f(a.x(), a.y());
    glVertex2f(b.x(), b.y());
    glVertex2f(c.x(), c.y());
    glVertex2f(d.x(), d.y());

    glEnd();
}

QPointF Canvas::normaliseMouseCoord(QPoint p)
{
    double maxRY = this->height();
    double maxRX = this->width();
    double minRX = 0;
    double minRY = 0;
    double minTX = 0;
    double minTY = 0;
    double maxTX = 100;
    double maxTY = 100;

    double mx = ((p.x() - minRX)/(maxRX - minRX))*(maxTX-minTX)+minTX;
    double my = (((p.y() - minRY)/(maxRY - minRY))*(maxTY-minTY)+minTY);

    return QPointF(mx,my);

}

void Canvas::checkBoundingBoxIntersect(QPointF p)
{
    //qDebug() << "Checking ----------------------------";

    std::vector<Node> nodes = o_diagram ->GetNodes();
    bool intersect = false ;

    for(auto &it : nodes)
    {
        QRectF box = it.GetRectContainer();

        if(box.contains(p))
        {
            intersect = true;
            qDebug() << it.GetCartCentre();
            drawCentreOfArc(it.GetCartCentre());

        }else
        {
            intersect = false;
        }

        it.SetHighlighted(intersect);
    }
    o_diagram->SetNodes(nodes);
    repaint();
    //update();

}

void Canvas::mouseMoveEvent(QMouseEvent *event)
{
//    if(event->buttons() == Qt::LeftButton)
//    {
//        qDebug() << "Clicked";
//        SetClickedPoint(normaliseMouseCoord(event->pos()));
//    }
    QPointF pos = normaliseMouseCoord(event->pos());
    QToolTip::showText(event->screenPos().toPoint(), QString::number(pos.x()) + " , " + QString::number(pos.y()), this);
    checkBoundingBoxIntersect(normaliseMouseCoord(event->pos()));


}

void Canvas::boundingBoxesCheckboxToggle(bool toggled)
{
    SetBoundingBoxToggle(toggled);
    update();
}

void Canvas::centerOfArcsCheckboxToggle(bool toggled)
{
    SetCenterOfArcsToggle(toggled);
    update();
}

void Canvas::SetBoundingBoxToggle(bool t)
{
    m_boundingBoxToggle = t;
}

bool Canvas::GetBoundingBoxToggle()
{
    return m_boundingBoxToggle;
}

void Canvas::SetCenterOfArcsToggle(bool t)
{
   m_centerOfArcsToggle = t;
}

bool Canvas::GetCenterOfArcsToggle()
{
    return m_centerOfArcsToggle;
}

