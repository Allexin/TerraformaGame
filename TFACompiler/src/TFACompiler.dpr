program TFACompiler;

{$APPTYPE CONSOLE}

{$R *.res}

uses
  System.SysUtils,
  TFAUtils in 'TFAUtils.pas',
  uOSAL in 'uOSAL.pas';

Procedure CompileMapsToTFA();
var
  HeightMapFileName:string;
  TextureFileName:string;
  TFAFileName:string;
  TFAFile:TTFAFile;
begin
  if (ParamCount=0) then begin
    Writeln('please pass correct args:');
    Writeln(' <tiff 16-bit heightmap file>');
    Exit;
  end;

  HeightMapFileName:=ParamStr(1);
  TextureFileName:=ChangeFileExt(HeightMapFileName,'.png');
  if not FileExists(TextureFileName) then
    TextureFileName:=ChangeFileExt(HeightMapFileName,'.bmp');
  if not FileExists(TextureFileName) then
    TextureFileName:=ChangeFileExt(HeightMapFileName,'.bmpx');

  TFAFileName:=ChangeFileExt(HeightMapFileName,'.tfa');

  if not FileExists(HeightMapFileName) then begin
    writeln('heightmap file not found'+HeightMapFileName);
    Exit;
  end;

  if not FileExists(TextureFileName) then begin
    writeln('texture file not found(png,bmp,bmpx supported)'+TextureFileName);
    Exit;
  end;

  TFAFile:=TTFAFile.Create();

  if (not TFAFile.ImportHeightMapTIFF(HeightMapFileName)) then begin
    writeln('Can''t open heightmap file:'+TFAFile.LastError);
    TFAFile.Free();
    Exit;
  end;

  if (not TFAFile.ImportTextureBMP(TextureFileName)) then begin
    writeln('Can''t open texture file:'+TFAFile.LastError);
    TFAFile.Free();
    Exit;
  end;

  if (not TFAFile.SaveToFile(TFAFileName)) then begin
    writeln('Can''t save tfa file:'+TFAFile.LastError);
    TFAFile.Free();
    Exit;
  end;

  TFAFile.Free();
  Writeln('tfa file compiled');
end;




begin
  CompileMapsToTFA();
end.
