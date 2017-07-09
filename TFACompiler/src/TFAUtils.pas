unit TFAUtils;

interface

uses
  Winapi.Windows, System.SysUtils, System.Classes, Vcl.Graphics, PNGImage,
  LibTiffDelphi, uOSAL;

type
  TTFAFile = class
  protected
    FHeightmap:PWord;
    FHeightMapWidth:integer;
    FHeightMapHeight:integer;

    FTexture:PWord;
    FTextureWidth:integer;
    FTextureHeight:integer;


    Function GetChunkLayerCoord(chunkX, chunkY, x, y: integer; var outCoord:TPoint):boolean;
  public
    LastError:string;

    Constructor Create();

    Function ImportHeightMapTIFF(const FileName:string):boolean;
    Function ImportTextureBMP(const FileName:string):boolean;

    Function SaveToFile(const FileName:string):boolean;
  end;

implementation

{ TTFAFile }

const
  CHUNK_RESOLUTION:integer = 64;

constructor TTFAFile.Create;
begin
  FHeightmap:=nil;
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
      a:=255;

      if srcA<>nil then
        if srcA[x]<>255 then
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

const
  TFA_FILE_VERSION:integer = 1;

function TTFAFile.GetChunkLayerCoord(chunkX, chunkY, x, y: integer; var outCoord:TPoint):boolean ;
begin
  outCoord.X:=chunkX*CHUNK_RESOLUTION + x;
  outCoord.Y:=chunkY*CHUNK_RESOLUTION + y;
  Result:=(outCoord.X>=0) and (outCoord.X<FHeightMapWidth) and (outCoord.Y>=0) and (outCoord.Y<FHeightMapHeight);
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

  if (FTextureWidth<>FHeightMapWidth) or (FTextureHeight<>FHeightMapHeight) then begin
    LastError:='texture and heightmap size not equal';
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
      for iy := -1  to CHUNK_RESOLUTION  do
        for ix := -1  to CHUNK_RESOLUTION  do begin
          if (GetChunkLayerCoord(x,y, ix,iy, coord)) then begin
            It:=FHeightmap;
            inc(It,coord.Y*FHeightMapHeight + coord.X);
            height := It^;
          end
          else
            height:=0;
          F.Write(@height,sizeof(word));
        end;
  for y := 0 to chunksYCount-1 do
    for x := 0 to chunksYCount-1 do
      for iy := -1  to CHUNK_RESOLUTION  do
        for ix := -1  to CHUNK_RESOLUTION  do begin
          if (GetChunkLayerCoord(x,y, ix,iy, coord)) then begin
            TIt:=PByte(FTexture);
            inc(TIt,coord.Y*FHeightMapHeight*4 + coord.X*4);
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

  F.Free();
  Result:=true;
end;

end.
