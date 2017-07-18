unit TFAUtils;

interface

uses
  Winapi.Windows, System.SysUtils, System.Classes, Vcl.Graphics, PNGImage,
  LibTiffDelphi, uOSAL;

type
{//REMOVE NORMAL MAP CALCULATION
  TVector3f = record
    x,y,z:single;
    class operator Add(a,b:TVector3f):TVector3f;
    class operator Subtract(const a,b:TVector3f):TVector3f;
    class operator Multiply(const a,b:TVector3f):TVector3f;
    class operator Multiply(const a:TVector3f; s:single):TVector3f;
    Function Dot(a:TVector3f):single;
    Procedure Normalize();
  end;                          }

  TTFAFile = class
  protected
    FHeightmap:PWord;
    {//REMOVE NORMAL MAP CALCULATION
    FNormalmap:PInteger;
    }
    FHeightMapWidth:integer;
    FHeightMapHeight:integer;

    FTexture:PWord;
    FTextureWidth:integer;
    FTextureHeight:integer;


    Function GetChunkLayerCoord(chunkX, chunkY, x, y: integer; var outCoord:TPoint):boolean;
    Function GetChunkTexLayerCoord(chunkX, chunkY, x, y: integer; var outCoord:TPoint):boolean;
    {//REMOVE NORMAL MAP CALCULATION
    Function GetNormal(chunkX, chunkY, x, y: integer): TVector3f;
    }
  public
    LastError:string;

    Constructor Create();

    Function ImportHeightMapTIFF(const FileName:string):boolean;
    Function ImportTextureBMP(const FileName:string):boolean;
    {//REMOVE NORMAL MAP CALCULATION
    Procedure GenerateNormalMap();
    }

    Function SaveToFile(const FileName:string):boolean;
  end;



implementation


{ TVector3f }
{//REMOVE NORMAL MAP CALCULATION
class operator TVector3f.Add(a, b: TVector3f): TVector3f;
begin
  Result.x := a.x + b.x;
	Result.y := a.y + b.y;
	Result.z := a.z + b.z;
end;

function TVector3f.Dot(a: TVector3f): single;
begin
  Result:=((x* a.x) + (y* a.y) + (z* a.z));
end;

class operator TVector3f.Multiply(const a: TVector3f; s: single): TVector3f;
begin
  Result.x := a.x *s;
	Result.y := a.y *s;
	Result.z := a.z *s;
end;

class operator TVector3f.Multiply(const a, b: TVector3f): TVector3f;
begin
  Result.x := (a.y  * b.z) - (a.z  * b.y);
	Result.y := (a.z  * b.x) - (a.x  * b.z);
	Result.z := (a.x  * b.y) - (a.y  * b.x);
end;

procedure TVector3f.Normalize;
var
  l:single;
begin
  if (x=0) and (y=0) and (z=0) then
		Exit;
	l := sqrt(sqr(x)+sqr(y)+sqr(z));
	x := x/l;
	y := y/l;
	z := z/l;
end;

class operator TVector3f.Subtract(const a, b: TVector3f): TVector3f;
begin
  Result.x := a.x - b.x;
	Result.y := a.y - b.y;
	Result.z := a.z - b.z;
end;

Function Vector3f(x,y,z:single):TVector3f;
begin
  Result.x:=x;
  Result.y:=y;
  Result.z:=z;
end;
}

{ TTFAFile }

const
  CHUNK_RESOLUTION:integer = 64;
const
  TFA_FILE_VERSION:integer = 5;

constructor TTFAFile.Create;
begin
  FHeightmap:=nil;
  {//REMOVE NORMAL MAP CALCULATION
  FNormalmap:=nil;
  }
  FTexture:=nil;
end;

function TTFAFile.ImportHeightMapTIFF(const FileName: string): boolean;
var
  F:PTIFF;
  width,height:integer;
  bpp:byte;
  channels:byte;
  It:PWord;
  y:integer;
begin
  Result:=false;
  F:=TIFFOpen(FileName,'r');
  TIFFGetField(F, TIFFTAG_IMAGEWIDTH,@width);
  TIFFGetField(F, TIFFTAG_IMAGELENGTH,@height);
  TIFFGetField(F, TIFFTAG_SAMPLESPERPIXEL,@channels);
  TIFFGetField(F, TIFFTAG_BITSPERSAMPLE,@bpp);
  if (width mod CHUNK_RESOLUTION <> 0) then begin
    LastError:='Incorrect image width='+IntToStr(width)+', must be multiple of '+IntToStr(CHUNK_RESOLUTION);
    Exit;
  end;
  if (height mod CHUNK_RESOLUTION <> 0) then begin
    LastError:='Incorrect image height='+IntToStr(height)+', must be multiple of '+IntToStr(CHUNK_RESOLUTION);
    Exit;
  end;

  if (channels <> 1) then begin
    LastError:='Incorrect samples per pixel='+IntToStr(channels)+', must be 1';
    Exit;
  end;

  if (bpp <> 16) then begin
    LastError:='Incorrect bits per sample='+IntToStr(bpp)+', must be 16';
    Exit;
  end;

  if (FHeightmap<>nil) then
    FreeMemory(FHeightmap);

  FHeightMapWidth:=width;
  FHeightMapHeight:=height;
  FHeightmap:=GetMemory(width*sizeof(word)*height);

  It:=FHeightmap;
  for y := 0 to height-1 do begin
    TIFFReadScanline(F, It, y, 0);
    inc(It,width);
  end;

  TIFFClose(F);
  Result:=true;
end;

Function LoadItemPNG(const FileName:string):TBitMap;
type
  TPNGColor = packed record
    r,g,b,a:byte;
  end;
var
  Image:TPngImage;
  x,y:integer;
  dest:PByte;
  srcRGB:Pbyte;
  srcA:PByteArray;
  r,g,b,a:byte;
  Data,It:^TPNGColor;
begin

  Image:=TPngImage.Create();
  Image.LoadFromFile(FileName);
  Result:=TBitMap.Create();
  Result.Width:=Image.Width;
  Result.Height:=Image.Height;
  Result.PixelFormat:=pf32bit;
  for y := 0 to Result.Height-1 do begin
    dest:=Result.ScanLine[y];
    srcRGB:=Image.Scanline[y];
    srcA:=Image.AlphaScanline[y];
    for x := 0 to Result.Width-1 do begin
      b:=srcRGB^;inc(srcRGB);
      g:=srcRGB^;inc(srcRGB);
      r:=srcRGB^;inc(srcRGB);
      a:=0;

      if srcA<>nil then
          a:=srcA[x];

      dest^:=b; inc(dest);
      dest^:=g; inc(dest);
      dest^:=r; inc(dest);
      dest^:=a; inc(dest);
    end;
  end;
  Image.Free();
end;

function TTFAFile.ImportTextureBMP(const FileName: string): boolean;
var
  b:TBitMap;
  scanLine:PByteArray;
  It:PByte;
  y:integer;
begin
  if UpperCase(ExtractFileExt(FileName))='.PNG' then begin
    b:=LoadItemPNG(FileName);
  end
  else begin
    b:=TBitMap.Create();
    b.LoadFromFile(FileName);
  end;
  b.PixelFormat:=pf32bit;
  FTextureWidth:=b.Width;
  FTextureHeight:=b.Height;

  if (FTexture<>nil) then
    FreeMemory(FTexture);

  FTexture:=GetMemory(FTextureWidth*FTextureHeight*4);
  It:=PByte(FTexture);
  for y := 0 to FTextureHeight-1 do begin
    scanLine:=b.ScanLine[y];
    CopyMemory(It,scanLine,FTextureWidth*4);
    inc(It,FTextureWidth*4);
  end;
  b.Free();

  Result:=true;
end;


{//REMOVE NORMAL MAP CALCULATION

procedure TTFAFile.GenerateNormalMap;
var
  chunksXCount,chunksYCount:integer;
  x,y, ix,iy:integer;
  Normal:PByte;
  coord:TPoint;
  N:TVector3f;
begin
  if (FNormalmap<>nil) then
    FreeMemory(FNormalmap);
  FNormalmap:=GetMemory(FHeightMapWidth*FHeightMapHeight*4);

  chunksXCount:=FHeightMapWidth div CHUNK_RESOLUTION;
  chunksYCount:=FHeightMapHeight div CHUNK_RESOLUTION;
  for y := 0 to chunksYCount-1 do
    for x := 0 to chunksYCount-1 do

      for iy := 0  to CHUNK_RESOLUTION-1  do
        for ix := 0  to CHUNK_RESOLUTION-1  do begin
          if (GetChunkLayerCoord(x,y, ix,iy, coord)) then begin
            Normal:=PByte(FNormalmap);
            inc(Normal,coord.Y*FHeightMapHeight*4 + coord.X*4);
            N:=GetNormal(x,y, ix,iy);
            Normal^:=trunc((N.x+1)*127);inc(Normal);
            Normal^:=trunc((N.y+1)*127);inc(Normal);
            Normal^:=trunc((N.z+1)*127);inc(Normal);
          end
          else begin
            //WTF??
          end;
        end;
end;

Function Normal(const v1,v2,v3:TVector3f):Tvector3f;
begin
  Result := ((v3 - v1)*(v2 - v1));
  Result.Normalize;
end;

Function TTFAFile.GetNormal(chunkX, chunkY, x, y: integer): TVector3f;
type
  THeightInfo = record
    height:word;
    vertex:TVector3f;
    available:boolean;
  end;
Function GetHeight(ix,iy:integer):THeightInfo;
var
  coord:TPoint;
  It:PWord;
begin
  Result.available:=GetChunkLayerCoord(chunkX,chunkY, ix,iy, coord);
  if (Result.available) then begin
    It:=FHeightmap;
    inc(It,coord.Y*FHeightMapHeight + coord.X);
    Result.height:=It^;
  end;
end;
var
  minusX:THeightInfo;
  minusY:THeightInfo;
  plusX:THeightInfo;
  plusY:THeightInfo;
  center:THeightInfo;
begin
  center:=GetHeight(x,y);
  center.vertex:=Vector3f(0,0,center.height/65535*256*25);
  minusX:=GetHeight(x-1,y);
  minusX.vertex:=Vector3f(-1000/64,0,minusX.height/65535*256*25);
  minusY:=GetHeight(x,y-1);
  minusY.vertex:=Vector3f(0,-1000/64,minusY.height/65535*256*25);
  plusX:=GetHeight(x+1,y);
  plusX.vertex:=Vector3f(+1000/64,0,plusX.height/65535*256*25);
  plusY:=GetHeight(x,y+1);
  plusY.vertex:=Vector3f(0,+1000/64,plusY.height/65535*256*25);
  Result:=Vector3f(0,0,1);

  if (minusX.available) and (minusY.available) then
    Result:=Result + Normal(center.vertex,minusY.vertex,minusX.vertex);
  if (plusX.available) and (minusY.available) then
    Result:=Result + Normal(center.vertex,plusX.vertex,minusY.vertex);
  if (plusX.available) and (plusY.available) then
    Result:=Result + Normal(center.vertex,plusY.vertex,plusX.vertex);
  if (minusX.available) and (plusY.available) then
    Result:=Result + Normal(center.vertex,minusX.vertex,plusY.vertex);
  Result.Normalize();
end;
}

function TTFAFile.GetChunkLayerCoord(chunkX, chunkY, x, y: integer; var outCoord:TPoint):boolean ;
begin
  outCoord.X:=chunkX*CHUNK_RESOLUTION + x;
  outCoord.Y:=chunkY*CHUNK_RESOLUTION + y;
  Result:=(outCoord.X>=0) and (outCoord.X<FHeightMapWidth) and (outCoord.Y>=0) and (outCoord.Y<FHeightMapHeight);
end;

function TTFAFile.GetChunkTexLayerCoord(chunkX, chunkY, x, y: integer; var outCoord:TPoint):boolean ;
begin
  outCoord.X:=chunkX*CHUNK_RESOLUTION*2 + x;
  outCoord.Y:=chunkY*CHUNK_RESOLUTION*2 + y;
  Result:=(outCoord.X>=0) and (outCoord.X<FTextureWidth) and (outCoord.Y>=0) and (outCoord.Y<FTextureHeight);
end;



Function TTFAFile.SaveToFile(const FileName: string):boolean;
var
  F:TSimpleWriteFileBlockMap;
  chunksXCount,chunksYCount:integer;
  x,y, ix,iy:integer;
  height:word;
  coord:TPoint;
  It:PWord;
  TIt:PByte;
  NIt:PByte;
  color:TColor;
  r,g,b,reserv:byte;
begin
  Result:=false;
  if (FHeightmap=nil) then begin
    LastError:='heightmap not loaded';
    Exit;
  end;
  if (FTexture=nil) then begin
    LastError:='texture not loaded';
    Exit;
  end;

  if (FTextureWidth<>FHeightMapWidth*2) or (FTextureHeight<>FHeightMapHeight*2) then begin
    LastError:='texture must have x2 heightmap size';
    Exit;
  end;

  F:=TSimpleWriteFileBlockMap.Create(FileName);
  F.WriteInteger(TFA_FILE_VERSION);
  chunksXCount:=FHeightMapWidth div CHUNK_RESOLUTION;
  chunksYCount:=FHeightMapHeight div CHUNK_RESOLUTION;
  F.WriteInteger(chunksXCount);
  F.WriteInteger(chunksYCount);
  for y := 0 to chunksYCount-1 do
    for x := 0 to chunksYCount-1 do
      for iy := -1  to CHUNK_RESOLUTION+1 +1  do
        for ix := -1  to CHUNK_RESOLUTION+1 +1  do begin
          if (GetChunkLayerCoord(x,y, ix,iy, coord)) then begin
            It:=FHeightmap;
            inc(It,coord.Y*FHeightMapWidth + coord.X);
            height := It^;
          end
          else
            height:=0;
          F.Write(@height,sizeof(word));
        end;
  for y := 0 to chunksYCount-1 do
    for x := 0 to chunksYCount-1 do
      for iy := -1  to CHUNK_RESOLUTION*2+1 +1  do
        for ix := -1  to CHUNK_RESOLUTION*2+1 +1  do begin
          if (GetChunkTexLayerCoord(x,y, ix,iy, coord)) then begin
            TIt:=PByte(FTexture);
            inc(TIt,coord.Y*FTextureWidth*4 + coord.X*4);
            b:=TIt^;inc(TIt);
            g:=TIt^;inc(TIt);
            r:=TIt^;inc(TIt);
            reserv:=TIt^;inc(TIt);
          end
          else begin
            r:=0;
            g:=0;
            b:=0;
            reserv:=0;
          end;

          F.WriteByte(r);
          F.WriteByte(g);
          F.WriteByte(b);
          F.WriteByte(reserv);
        end;
  {//REMOVE NORMAL MAP CALCULATION
  for y := 0 to chunksYCount-1 do
    for x := 0 to chunksYCount-1 do
      for iy := -1  to CHUNK_RESOLUTION  do
        for ix := -1  to CHUNK_RESOLUTION  do begin
          if (GetChunkLayerCoord(x,y, ix,iy, coord)) then begin
            NIt:=PByte(FNormalmap);
            inc(NIt,coord.Y*FHeightMapWidth*4 + coord.X*4);
            r:=NIt^;inc(NIt);
            g:=NIt^;inc(NIt);
            b:=NIt^;inc(NIt);
            reserv:=NIt^;inc(NIt);
          end
          else begin
            r:=128;
            g:=128;
            b:=255;
            reserv:=0;
          end;

          F.WriteByte(r);
          F.WriteByte(g);
          F.WriteByte(b);
          F.WriteByte(reserv);
        end;
        }

  F.Free();
  Result:=true;
end;

end.
