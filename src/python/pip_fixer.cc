#include<string>
#include<linalg.h>

std::string commandline_commands;


bool linsystem( Vector3d v1,Vector3d v2,Vector3d v3,Vector3d pt,Vector3d &res,double *detptr)
{
        double det,ad11,ad12,ad13,ad21,ad22,ad23,ad31,ad32,ad33;
        det=v1[0]*(v2[1]*v3[2]-v3[1]*v2[2])-v1[1]*(v2[0]*v3[2]-v3[0]*v2[2])+v1[2]*(v2[0]*v3[1]-v3[0]*v2[1]);
        if(detptr != nullptr) *detptr=det;
        ad11=v2[1]*v3[2]-v3[1]*v2[2];
        ad12=v3[0]*v2[2]-v2[0]*v3[2];
        ad13=v2[0]*v3[1]-v3[0]*v2[1];
        ad21=v3[1]*v1[2]-v1[1]*v3[2];
        ad22=v1[0]*v3[2]-v3[0]*v1[2];
        ad23=v3[0]*v1[1]-v1[0]*v3[1];
        ad31=v1[1]*v2[2]-v2[1]*v1[2];
        ad32=v2[0]*v1[2]-v1[0]*v2[2];
        ad33=v1[0]*v2[1]-v2[0]*v1[1];

        if(fabs(det) < 0.00001)
                return true;
        
        res[0] = (ad11*pt[0]+ad12*pt[1]+ad13*pt[2])/det;
        res[1] = (ad21*pt[0]+ad22*pt[1]+ad23*pt[2])/det;
        res[2] = (ad31*pt[0]+ad32*pt[1]+ad33*pt[2])/det;
        return false;
}

int curl_download(std::string url, std::string path)
{
	return 0;
}
