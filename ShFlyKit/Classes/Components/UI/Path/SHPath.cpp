//
//  SHLabelLine.cpp
//  SHLabel
//
//  Created by hsh on 19/8/22.
//  Copyright © 2019年 hsh All rights reserved.
//


#include "SHPath.h"
#include <math.h>


#pragma mark - SHPath-----路径基类
SHPath::SHPath(SHPoint point){
    m_lastPath = nullptr;
    m_nextPath = nullptr;
    m_endPoint = {point.x, point.y};
    m_needsUpdate = true;
    m_pointBuffer = nullptr;
}


SHPath::~SHPath(){
    if (m_lastPath != nullptr) {
        // 判断上一条路径的下一条路径指的是否还是自己
        if (m_lastPath->m_nextPath == this) {
            // 防止野指针
            m_lastPath->m_nextPath = nullptr;
        }
    }
    if (m_nextPath != nullptr) {
        delete m_nextPath;
    }
    if (m_pointBuffer != nullptr) {
        delete m_pointBuffer;
    }
}


bool SHPath::appendPath(SHPath *path){
    if (path->m_lastPath != nullptr) {
        return false;
    }
    SHPath *endPath = this;
    while (endPath->m_nextPath != nullptr) {
        endPath = endPath->m_nextPath;
    }
    endPath->setNextPath(path);
    path->setLastPath(endPath);
    path->m_needsUpdate = true;
    return true;
}


bool SHPath::removeFrontPath(){
    if (this->m_lastPath == nullptr) {
        return false;
    }
    this->m_lastPath->m_nextPath = nullptr;
    this->m_lastPath = nullptr;
    this->m_needsUpdate = true;
    return true;
}


bool SHPath::removeBackPath(bool release){
    if (this->m_nextPath == nullptr) {
        return false;
    }
    if (release) {
        delete this->m_nextPath;
    }else {
        this->m_nextPath->m_lastPath = nullptr;
        this->m_nextPath->m_needsUpdate = true;
    }
    this->m_nextPath = nullptr;
    return true;
}


double SHPath::getLength(bool isTotal){
    double length = getSelfLength();
    if (isTotal && m_nextPath != nullptr) {
        length += m_nextPath->getLength(true);
    }
    return length;
}



void SHPath::getPosTan(double precision, std::vector<SHPoint> *outBuffer){
    if (m_needsUpdate) {
        if (m_lastPath != nullptr) {
            updatePosTan(precision);
        }else {
            if (m_pointBuffer != nullptr) {
                delete m_pointBuffer;
            }
            m_pointBuffer = new std::vector<SHPoint>(0);
        }
        m_needsUpdate = false;
    }
    outBuffer->insert(outBuffer->end(), m_pointBuffer->begin(), m_pointBuffer->end());
    
    if (this->m_nextPath != nullptr) {
        this->m_nextPath->getPosTan(precision, outBuffer);
    }
}


void SHPath::setNeedsUpdate(){
    for (SHPath *path = this; path != nullptr; path = getNextPath()) {
        path->m_needsUpdate = true;
    }
}


double SHPath::pointSpace(SHPoint point1, SHPoint point2){
    double xSpace = point1.x - point2.x;
    double ySpace = point1.y - point2.y;
    return sqrt(xSpace * xSpace + ySpace * ySpace);
}


#pragma mark - SHPointPath-----点
SHPointPath::SHPointPath(SHPoint point) : SHPath(point){
    
}


SHPointPath* SHPointPath::clone(bool needsUpdate){
    SHPointPath *clone = new SHPointPath(m_endPoint);
    clone->m_needsUpdate = needsUpdate | m_needsUpdate;
    clone->m_pointBuffer = new std::vector<SHPoint>(*m_pointBuffer);
    if (getNextPath() != nullptr) {
        clone->appendPath(getNextPath()->clone(false));
    }
    return clone;
}


double SHPointPath::getSelfLength(){
    return 0.0;
}


void SHPointPath::updatePosTan(double precision){
    if (m_pointBuffer == nullptr) {
        m_pointBuffer = new std::vector<SHPoint>(1);
    }
    (*m_pointBuffer)[0] = m_endPoint;
}


#pragma mark - SHLinePath-----直线
SHLinePath::SHLinePath(SHPoint point) : SHPath(point){
    
}


SHLinePath* SHLinePath::clone(bool needsUpdate){
    SHLinePath *clone = new SHLinePath(m_endPoint);
    clone->m_needsUpdate = needsUpdate | m_needsUpdate;
    clone->m_pointBuffer = new std::vector<SHPoint>(*m_pointBuffer);
    if (getNextPath() != nullptr) {
        clone->appendPath(getNextPath()->clone(false));
    }
    return clone;
}


double SHLinePath::getSelfLength(){
    if (getLastPath() == nullptr) {
        return 0.0;
    }
    if (m_needsUpdate) {
        m_length = pointSpace(m_endPoint, getLastPath()->getEndPoint());
    }
    return m_length;
}


void SHLinePath::updatePosTan(double precision){
    double length = getSelfLength();
    int pointCount = static_cast<int>(length / precision);
    double xSpike = (m_endPoint.x - getLastPath()->getEndPoint().x) / length * precision;
    double ySpike = (m_endPoint.y - getLastPath()->getEndPoint().y) / length * precision;
    // 重新精确大小分配outBuffer的内存，提升内存使用率
    if (m_pointBuffer != nullptr) {
        delete m_pointBuffer;
    }
    m_pointBuffer = new std::vector<SHPoint>(pointCount);
    
    for (int i = 0; i < pointCount; i++) {
        (*m_pointBuffer)[i].x = getLastPath()->getEndPoint().x + i * xSpike;
        (*m_pointBuffer)[i].y = getLastPath()->getEndPoint().y + i * ySpike;
    }
}


#pragma mark - SHRoundPath-----圆
SHRoundPath::SHRoundPath(SHPoint centrePoint, double angle) : SHPath(centrePoint){
    m_angle = angle;
    m_centrePoint = centrePoint;
}


SHRoundPath* SHRoundPath::clone(bool needsUpdate){
    SHRoundPath *clone = new SHRoundPath(m_centrePoint, m_angle);
    clone->m_needsUpdate = needsUpdate | m_needsUpdate;
    clone->m_pointBuffer = new std::vector<SHPoint>(*m_pointBuffer);
    if (getNextPath() != nullptr) {
        clone->appendPath(getNextPath()->clone(false));
    }
    return clone;
}


void SHRoundPath::setLastPath(SHPath *lastPath){
    SHPath::setLastPath(lastPath);
    if (lastPath == nullptr) {
        m_radii = 0.0;
        m_beginAngle = 0.0;
        m_endPoint.x = m_centrePoint.x;
        m_endPoint.y = m_centrePoint.y;
    }else{
        m_radii = pointSpace(m_endPoint, getLastPath()->getEndPoint());
        m_beginAngle = atan((lastPath->getEndPoint().x - m_centrePoint.x) / (lastPath->getEndPoint().y - m_centrePoint.y));
        m_endPoint.x = m_centrePoint.x + m_radii * sin(m_beginAngle + m_angle);
        m_endPoint.y = m_centrePoint.y + m_radii * cos(m_beginAngle + m_angle);
    }
}


double SHRoundPath::getSelfLength(){
    if (getLastPath() == nullptr) {
        return 0.0;
    }
    if (m_needsUpdate) {
        m_length = fabs(m_angle) * m_radii;
    }
    return m_length;
}


void SHRoundPath::updatePosTan(double precision){
    double length = getSelfLength();
    int pointCount = static_cast<int>(length / precision);
    double angleSpike = m_angle / pointCount;
    // 重新精确大小分配outBuffer的内存，提升内存使用率
    if (m_pointBuffer != nullptr) {
        delete m_pointBuffer;
    }
    m_pointBuffer = new std::vector<SHPoint>(pointCount);
    
    for (int i = 0; i < pointCount; i++) {
        (*m_pointBuffer)[i].x = m_centrePoint.x + m_radii * sin(m_beginAngle + i * angleSpike);
        (*m_pointBuffer)[i].y = m_centrePoint.y + m_radii * cos(m_beginAngle + i * angleSpike);
    }
}


#pragma mark - SHBezierPath-----贝塞尔曲线
SHBezierPath::SHBezierPath(SHPoint anchorPoint, SHPoint endPoint) : SHPath(endPoint){
    m_anchorPoint = anchorPoint;
}


SHBezierPath* SHBezierPath::clone(bool needsUpdate){
    SHBezierPath *clone = new SHBezierPath(m_anchorPoint, m_endPoint);
    clone->m_needsUpdate = needsUpdate | m_needsUpdate;
    clone->m_pointBuffer = new std::vector<SHPoint>(*m_pointBuffer);
    if (getNextPath() != nullptr) {
        clone->appendPath(getNextPath()->clone(false));
    }
    return clone;
}


void SHBezierPath::setLastPath(SHPath *lastPath){
    SHPath::setLastPath(lastPath);
    if (lastPath == nullptr) {
        
    }else {
        m_ax = lastPath->getEndPoint().x - 2 * m_anchorPoint.x + m_endPoint.x;
        m_ay = lastPath->getEndPoint().y - 2 * m_anchorPoint.y + m_endPoint.y;
        m_bx = 2 * m_anchorPoint.x - 2 * lastPath->getEndPoint().x;
        m_by = 2 * m_anchorPoint.y - 2 * lastPath->getEndPoint().y;
        
        m_A = 4 * (m_ax * m_ax + m_ay * m_ay);
        m_B = 4 * (m_ax * m_bx + m_ay * m_by);
        m_C = m_bx * m_bx + m_by * m_by;
    }
}

//长度函数
/*
 L(t) = Integrate[s[t], t]
 
 L(t_) = ((2*Sqrt[A]*(2*A*t*Sqrt[C + t*(B + A*t)] + B*(-Sqrt[C] + Sqrt[C + t*(B + A*t)])) + (B^2 - 4*A*C) (Log[B + 2*Sqrt[A]*Sqrt[C]] - Log[B + 2*A*t + 2 Sqrt[A]*Sqrt[C + t*(B + A*t)]])) /(8* A^(3/2)));
 */
double SHBezierPath::getSelfLength(){
    if (getLastPath() == nullptr) {
        return 0.0;
    }
    if (m_needsUpdate) {
        double temp1 = sqrt(m_A + m_B + m_C);
        double temp2 = (2 * m_A * temp1 + m_B * (temp1 - sqrt(m_C)));
        double temp3 = log(m_B + 2 * sqrt(m_A) * sqrt(m_C));
        double temp4 = log(m_B + 2 * m_A + 2 * sqrt(m_A) * temp1);
        double temp5 = 2 * sqrt(m_A) * temp2;
        double temp6 = (m_B * m_B - 4 * m_A * m_C) * (temp3 - temp4);
        
        m_length = (temp5 + temp6) / (8 * pow(m_A, 1.5));
    }
    return m_length;
}


void SHBezierPath::updatePosTan(double precision){
    double length = getSelfLength();
    int pointCount = static_cast<int>(length / precision);
    // 重新精确大小分配outBuffer的内存，提升内存使用率
    if (m_pointBuffer != nullptr) {
        delete m_pointBuffer;
    }
    m_pointBuffer = new std::vector<SHPoint>(pointCount);
    
    for (int i = 0; i < pointCount; i++) {
        double t = (double)i / pointCount;
        //如果按照线形增长,此时对应的曲线长度
        double l = t * length;
        //根据L函数的反函数，求得l对应的t值
        t = invertLength(t, l);
        //根据贝塞尔曲线函数，求得取得此时的x,y坐标
        (*m_pointBuffer)[i].x = (1 - t) * (1 - t) * getLastPath()->getEndPoint().x
        + 2 * (1 - t) * t * m_anchorPoint.x
        + t * t * m_endPoint.x;
        (*m_pointBuffer)[i].y = (1 - t) * (1 - t) * getLastPath()->getEndPoint().y
        + 2 * (1 - t) * t * m_anchorPoint.y
        + t * t * m_endPoint.y;
    }
}


//长度函数反函数，使用牛顿切线法求解
/*
 X(n+1) = Xn - F(Xn)/F'(Xn)
 */
double SHBezierPath::invertLength(double t, double l){
    double t1 = t, t2;
    do{
        t2 = t1 - (getBezierLength(t1) - l) / speed(t1);
        if(fabs(t1 - t2) < 0.0001) break;
        t1 = t2;
    } while(true);
    return t2;
}


//速度函数
/*
 s(t_) = Sqrt[A*t*t+B*t+C]
 */
double SHBezierPath::speed(double t){
    if (t > 1.0) {
        t = 1.0;
    }else if (t <= 0.0) {
        t = 0.0;
    }
    return sqrt(m_A * t * t + m_B * t + m_C);
}


//长度函数
/*
 L(t) = Integrate[s[t], t]
 
 L(t_) = ((2*Sqrt[A]*(2*A*t*Sqrt[C + t*(B + A*t)] + B*(-Sqrt[C] + Sqrt[C + t*(B + A*t)])) + (B^2 - 4*A*C) (Log[B + 2*Sqrt[A]*Sqrt[C]] - Log[B + 2*A*t + 2 Sqrt[A]*Sqrt[C + t*(B + A*t)]])) /(8* A^(3/2)));
 */
double SHBezierPath::getBezierLength(double t){
    if (t > 1.0) {
        t = 1.0;
    }else if (t <= 0.0) {
        return 0.0;
    }
    double temp1 = sqrt(m_C + t * (m_B + m_A * t));
    double temp2 = (2 * m_A * t * temp1 + m_B * (temp1 - sqrt(m_C)));
    double temp3 = log(m_B + 2 * sqrt(m_A) * sqrt(m_C));
    double temp4 = log(m_B + 2 * m_A * t + 2 * sqrt(m_A) * temp1);
    double temp5 = 2 * sqrt(m_A) * temp2;
    double temp6 = (m_B * m_B - 4 * m_A * m_C) * (temp3 - temp4);
    
    return (temp5 + temp6) / (8 * pow(m_A, 1.5));
}


#pragma mark - SHCustomPath-----自定义曲线
SHCustomPath::SHCustomPath(std::vector<SHPoint> *customPoint) : SHPath(*customPoint->end()){
    m_customPoint = new std::vector<SHPoint>(*customPoint);
}


SHCustomPath::~SHCustomPath(){
    if (m_customPoint != nullptr) {
        delete m_customPoint;
    }
}


SHCustomPath* SHCustomPath::clone(bool needsUpdate){
    SHCustomPath *clone = new SHCustomPath(m_customPoint);
    clone->m_needsUpdate = needsUpdate | m_needsUpdate;
    clone->m_pointBuffer = new std::vector<SHPoint>(*m_pointBuffer);
    if (getNextPath() != nullptr) {
        clone->appendPath(getNextPath()->clone(false));
    }
    return clone;
}



double SHCustomPath::getSelfLength(){
    if (m_needsUpdate) {
        if (m_customPoint == nullptr || m_customPoint->size() == 0) {
            m_length = 0.0;
        }else {
            if (getLastPath() == nullptr) {
                m_length = 0.0;
            }else {
                m_length = pointSpace(getLastPath()->getEndPoint(), (*m_customPoint)[0]);
            }
            for (int i = 1; i < m_customPoint->size(); i++) {
                m_length += pointSpace((*m_customPoint)[i - 1], (*m_customPoint)[i]);
            }
        }
    }
    return m_length;
}


void SHCustomPath::updatePosTan(double precision){
    // 重新精确大小分配outBuffer的内存，提升内存使用率
    if (m_pointBuffer == nullptr) {
        m_pointBuffer = new std::vector<SHPoint>();
    }else {
        m_pointBuffer->clear();
    }
    if (m_customPoint != nullptr && m_customPoint->size() > 0) {
        double offset = 0.0;
        if (getLastPath() != nullptr) {
            offset = calcSegmentPoint(getLastPath()->getEndPoint(), (*m_customPoint)[0], precision, offset, m_pointBuffer);
        }
        for (int i = 1; i < m_customPoint->size(); i++) {
            offset = calcSegmentPoint((*m_customPoint)[i - 1], (*m_customPoint)[i], precision, offset, m_pointBuffer);
        }
    }    
}


double SHCustomPath::calcSegmentPoint(SHPoint point1, SHPoint point2, double precision, double offset, std::vector<SHPoint> *outBuffer)
{
    _LIBCPP_ASSERT(offset < precision, "偏移值 > 精度值");
    double length = pointSpace(point1, point2);
    if (length + offset < precision) {
        return length + offset;
    }
    double xSpace = point2.x - point1.x;
    double ySpace = point2.y - point1.y;
    double calcLength = 0.0;
    for (calcLength = precision - offset; calcLength < length; calcLength += precision) {
        double scale = calcLength / length;
        SHPoint point = {point1.x + xSpace * scale, point1.y + ySpace * scale};
        outBuffer->push_back(point);
    }
    return length - (calcLength - precision);
}
