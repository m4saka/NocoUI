#include <Siv3D.hpp>
#include <NocoUI.hpp>

void Main()
{
	noco::Init();

	// nocoファイルを読み込む
	const auto canvas = noco::Canvas::LoadFromFile(U"dialog.noco");

	// 必要に応じて文言を変更できる
	//canvas->setParamValue(U"dialogTitle", U"新たな仲間が参戦！");
	//canvas->setParamValue(U"messageText", U"仲間の名前を入力してください");

	// 初期値を設定
	canvas->setTextValueByTag(U"playerName", U"ノコタロウ");

	// フェードインのTweenを再生
	canvas->setTweenActiveByTag(U"in", true);

	bool isFinished = false;

	while (System::Update())
	{
		// Canvasの更新
		canvas->update();

		if (!isFinished)
		{
			// プレイヤー名が空でない場合のみ決定ボタンを有効化
			const String playerName = canvas->getTextValueByTag(U"playerName");
			canvas->setParamValue(U"decideButtonEnabled", playerName);

			// 決定ボタンクリックのイベント発火時
			if (canvas->isEventFiredWithTag(U"decideClicked") && !playerName.isEmpty())
			{
				// プレイヤー名を表示
				Print << U"プレイヤー名: {}"_fmt(playerName);

				// フェードアウトのTweenを再生
				canvas->setTweenActiveByTag(U"out", true);

				// 完了後は決定ボタンを無効化
				canvas->setParamValue(U"decideButtonEnabled", false);

				isFinished = true;
			}
		}

		// Canvasの描画
		canvas->draw();
	}
}
