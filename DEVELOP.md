# 開発用ドキュメント

## Gemの更新

まずは `lib/sdl2/version.rb` を更新してバージョンを上げる。その後

    rake gem
    
でパッケージが生成される。

    gem push pkg/ruby-sdl2-XXX.gem
    
でアップロードされる。`XXX`は適切なバージョン名に置き換える。


## API リファレンスの更新

API リファレンスは yard で書かれ、github pages でホスティングされている。

### リポジトリの準備

これは一回でよい。

    mkdir ruby-sdl2
    git clone git@github.com:ohai/ruby-sdl2.git master
    git clone -b gh-pages git@github.com:ohai/ruby-sdl2.git gh-pages

### yard のインストール

これも一回でよい。

    gem install yard

### ドキュメントの生成

    cd master
    rake doc
    
`master/doc/doc-en` 以下に生成される。

### 更新

    cd ../gh-pages/
    cp -r ../master/doc/doc-en .
    git add doc-en
    git commit -m "Update API reference"
    git push 
